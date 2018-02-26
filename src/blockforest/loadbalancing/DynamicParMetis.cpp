//======================================================================================================================
//
//  This file is part of waLBerla. waLBerla is free software: you can
//  redistribute it and/or modify it under the terms of the GNU General Public
//  License as published by the Free Software Foundation, either version 3 of
//  the License, or (at your option) any later version.
//
//  waLBerla is distributed in the hope that it will be useful, but WITHOUT
//  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
//  for more details.
//
//  You should have received a copy of the GNU General Public License along
//  with waLBerla (see COPYING.txt). If not, see <http://www.gnu.org/licenses/>.
//
//! \file DynamicParMetis.cpp
//! \ingroup blockforest
//! \author Christian Godenschwager <christian.godenschwager@fau.de>
//
//======================================================================================================================

#include "DynamicParMetis.h"

#include "core/load_balancing/ParMetisWrapper.h"

#include "core/logging/Logging.h"

#include "core/mpi/BufferSystem.h"
#include "core/mpi/MPIHelper.h"
#include "core/mpi/MPIManager.h"
#include "core/mpi/Gather.h"
#include "core/mpi/Gatherv.h"

#include "core/timing/Timer.h"

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/trim.hpp>

namespace walberla {
namespace blockforest {

std::pair<uint_t, uint_t> getBlockSequenceRange( const PhantomBlockForest & phantomForest, MPI_Comm comm )
{
   const uint_t rank = uint_c(mpi::translateRank(mpi::MPIManager::instance()->comm(), comm, MPIManager::instance()->rank()));

   uint_t numLocalBlocks = phantomForest.getNumberOfBlocks();

   size_t sequenceStartOnProcess = 0;
   MPI_Exscan( &numLocalBlocks, &sequenceStartOnProcess, 1, MPITrait<uint_t>::type(), MPI_SUM, comm );
   if( rank == 0 )
      sequenceStartOnProcess = uint_t( 0 );

   return std::make_pair( sequenceStartOnProcess, sequenceStartOnProcess + numLocalBlocks );
}

std::map< blockforest::BlockID, uint_t > getBlockIdToSequenceMapping( const PhantomBlockForest & phantomForest, const std::pair<uint_t, uint_t> & blockSequenceRange, MPI_Comm comm )
{
   std::map< blockforest::BlockID, uint_t > mapping;

   const auto & blockMap = phantomForest.getBlockMap();
   uint_t sequenceId = blockSequenceRange.first;
   for( auto it = blockMap.begin(); it != blockMap.end(); ++it )
      mapping.insert( std::make_pair( it->first, sequenceId++ ) );
   WALBERLA_ASSERT_EQUAL( sequenceId, blockSequenceRange.second );

   const std::vector<uint_t> neighborProcesses = phantomForest.getNeighboringProcesses();
   
   mpi::BufferSystem bs( comm );

   for( auto it = neighborProcesses.begin(); it != neighborProcesses.end(); ++it )
      bs.sendBuffer( mpi::translateRank(mpi::MPIManager::instance()->comm(), comm, int_c(*it)) ) << mapping;

   bs.setReceiverInfoFromSendBufferState( false, true );

   bs.sendAll();

   for( auto it = bs.begin(); it != bs.end(); ++it )
   {
      std::map< blockforest::BlockID, uint_t > remoteMapping;
      it.buffer() >> remoteMapping;

      for( auto remoteIt = remoteMapping.begin(); remoteIt != remoteMapping.end(); ++remoteIt )
      {
         auto result = mapping.insert( *remoteIt );
         WALBERLA_UNUSED( result );
         WALBERLA_ASSERT( result.second, "BlockId should be unique!" );
      }
   }

   return mapping;
}

template< typename T >
T * ptr( std::vector<T> & v )
{
   if( v.empty() )
      return NULL;
   else
      return &( v.front() );
}

typedef uint_t idx_t;

bool DynamicParMetis::operator()( std::vector< std::pair< const PhantomBlock *, uint_t > > & targetProcess,
                                  std::set< uint_t > & processesToRecvFrom,
                                  const PhantomBlockForest & phantomForest,
                                  const uint_t /*iteration*/ ) const
{
   WcTimer globalTimer;
   WcTimer parmetisTimer;
   globalTimer.start();

   //create new communicator which excludes processes which do not have blocks
   MPI_Comm subComm;
   MPI_Group allGroup, subGroup;
   MPI_Comm_group( MPIManager::instance()->comm(), &allGroup );
   std::vector<int> ranks;
   if (phantomForest.getNumberOfBlocks() > 0)
      ranks.push_back( MPIManager::instance()->rank() );
   ranks = mpi::allGatherv( ranks, MPIManager::instance()->comm() );
   auto numSubProcesses = ranks.size();
   WALBERLA_UNUSED(numSubProcesses);
   MPI_Group_incl(allGroup, int_c(ranks.size()), &ranks[0], &subGroup);
   MPI_Comm_create( MPIManager::instance()->comm(), subGroup, &subComm);

   int64_t edgecut = 0;
   std::vector<int64_t> part( phantomForest.getNumberOfBlocks(), int64_c( MPIManager::instance()->rank() ) );

   if (subComm != MPI_COMM_NULL)
   {
      const std::pair<uint_t, uint_t> blockSequenceRange = getBlockSequenceRange( phantomForest, subComm );
      const std::map< blockforest::BlockID, uint_t > mapping = getBlockIdToSequenceMapping( phantomForest, blockSequenceRange, subComm );

      std::vector<int64_t> vtxdist = mpi::allGather( int64_c( blockSequenceRange.second ), subComm );
      vtxdist.insert( vtxdist.begin(), uint_t( 0 ) );

      std::vector<int64_t> adjncy, xadj, vsize, vwgt, adjwgt;
      std::vector<double> xyz;

      for( auto it = targetProcess.begin(); it != targetProcess.end(); ++it )
      {
         xadj.push_back( int64_c( adjncy.size() ) );
         const PhantomBlock & block = *( it->first );
         auto bi = block.getData< DynamicParMetisBlockInfo >();

         switch( edgeSource_ )
         {
         case PARMETIS_EDGES_FROM_FOREST:
            for( auto nit = block.getNeighborhood().begin(); nit != block.getNeighborhood().end(); ++nit )
            {
               auto mapIt = mapping.find( nit->getId() );
               WALBERLA_ASSERT_UNEQUAL( mapIt, mapping.end(), "BlockId of neighbor is not contained in sequence mapping!" );
               adjncy.push_back( int64_c( mapIt->second ) );
               auto edgeWeightIt = bi.getEdgeWeights().find( nit->getId() );
               adjwgt.push_back( edgeWeightIt == bi.getEdgeWeights().end() ? int64_t( 0 ) : edgeWeightIt->second );
            }
            break;
         case PARMETIS_EDGES_FROM_EDGE_WEIGHTS:
            for( auto edgeIt = bi.getEdgeWeights().begin(); edgeIt != bi.getEdgeWeights().end(); ++edgeIt )
            {
               auto mapIt = mapping.find( edgeIt->first );
               WALBERLA_ASSERT_UNEQUAL( mapIt, mapping.end(), "BlockId of neighbor is not contained in sequence mapping!" );
               adjncy.push_back( int64_c( mapIt->second ) );
               adjwgt.push_back( edgeIt->second );
            }
         }
         vwgt.push_back( bi.getVertexWeight() );
         vsize.push_back( bi.getVertexSize() );
         xyz.push_back( bi.getVertexCoords()[0] );
         xyz.push_back( bi.getVertexCoords()[1] );
         xyz.push_back( bi.getVertexCoords()[2] );
      }
      xadj.push_back( int64_c( adjncy.size() ) );

      WALBERLA_ASSERT_EQUAL( vtxdist.size(), numSubProcesses + uint_t( 1 ) );
      WALBERLA_ASSERT_EQUAL( xadj.size(), phantomForest.getNumberOfBlocks() + 1 );
      WALBERLA_ASSERT_EQUAL( vwgt.size(), phantomForest.getNumberOfBlocks() );
      WALBERLA_ASSERT_EQUAL( vsize.size(), phantomForest.getNumberOfBlocks() );
      WALBERLA_ASSERT_EQUAL( adjncy.size(), adjwgt.size() );

      int64_t wgtflag = weightsToUse_;
      int64_t numflag = 0; // C-style ordering
      int64_t ncon = 1; // Number of constraints
      int64_t ndims = 3; // Number of dimensions
      double ubvec[] = { real_t( 1.05 ) }; // imbalance tolerance
      int64_t nparts = int64_c( MPIManager::instance()->numProcesses() ); // number of subdomains
      double ipc2redist = real_t( 1000000.0 ); // compute repartitioning with low edge cut (set lower (down to 0.000001) to get minimal repartitioning )
      MPI_Comm comm = subComm; //MPIManager::instance()->comm();
      std::vector<double> tpwgts( uint_c(nparts * ncon), 1.0 / double_c( nparts ) ); // vertex weight fraction that is stored in a subdomain
      int64_t options[] = { int64_t( 1 ), int64_t( 0 ), int64_t( 23 ), int64_t( 1 ) };

      int metisResult = core::METIS_OK;

      switch( algorithm_ )
      {
      case PARMETIS_PART_GEOM_KWAY:
         parmetisTimer.start();
         metisResult = core::ParMETIS_V3_PartGeomKway( ptr( vtxdist ), ptr( xadj ), ptr( adjncy ), ptr( vwgt ), ptr( adjwgt ), &wgtflag, &numflag, &ndims, ptr( xyz ), &ncon, &nparts, ptr( tpwgts ), ubvec, options, &edgecut, ptr( part ), &comm );
         parmetisTimer.end();
         break;
      case PARMETIS_PART_KWAY:
         parmetisTimer.start();
         metisResult = core::ParMETIS_V3_PartKway( ptr( vtxdist ), ptr( xadj ), ptr( adjncy ), ptr( vwgt ), ptr( adjwgt ), &wgtflag, &numflag, &ncon, &nparts, ptr( tpwgts ), ubvec, options, &edgecut, ptr( part ), &comm );
         parmetisTimer.end();
         break;
      case PARMETIS_ADAPTIVE_REPART:
         parmetisTimer.start();
         metisResult = core::ParMETIS_V3_AdaptiveRepart( ptr( vtxdist ), ptr( xadj ), ptr( adjncy ), ptr( vwgt ), ptr( vsize ), ptr( adjwgt ), &wgtflag, &numflag, &ncon, &nparts, ptr( tpwgts ), ubvec, &ipc2redist, options, &edgecut, ptr( part ), &comm );
         parmetisTimer.end();
         break;
      case PARMETIS_REFINE_KWAY:
         parmetisTimer.start();
         metisResult = core::ParMETIS_V3_RefineKway( ptr( vtxdist ), ptr( xadj ), ptr( adjncy ), ptr( vwgt ), ptr( adjwgt ), &wgtflag, &numflag, &ncon, &nparts, ptr( tpwgts ), ubvec, options, &edgecut, ptr( part ), &comm );
         parmetisTimer.end();
         break;
      }

      if( metisResult != core::METIS_OK )
         WALBERLA_ABORT("ParMetis failed!");
   }

   // Determine which processes will receive a block from this process
   std::vector<uint8_t> isSendingBlockToProcess( uint_c(MPIManager::instance()->numProcesses()), uint8_t( 0 ) );
   for( auto it = part.begin(); it != part.end(); ++it )
   {
      WALBERLA_ASSERT_GREATER_EQUAL( *it, 0 );
      WALBERLA_ASSERT_LESS( *it, MPIManager::instance()->numProcesses() );
      isSendingBlockToProcess[uint_c(*it)] = uint8_t( 1 );
   }
   isSendingBlockToProcess[uint_c(MPIManager::instance()->rank())] = uint8_t( 0 );

   std::vector<uint8_t> isReceivingBlockFromProcess( uint_c(MPIManager::instance()->numProcesses()), uint8_t( 0 ) );
   MPI_Alltoall( ptr( isSendingBlockToProcess ), 1, MPITrait<uint8_t>::type(), ptr( isReceivingBlockFromProcess ), 1, MPITrait<uint8_t>::type(), MPIManager::instance()->comm() );
   for( uint_t i = 0; i < isReceivingBlockFromProcess.size(); ++i )
      if( isReceivingBlockFromProcess[i] == uint8_t( 1 ) )
         processesToRecvFrom.insert( i );

   // assign target processes to blocks

   uint_t i = 0;
   for( auto it = targetProcess.begin(); it != targetProcess.end(); ++it, ++i )
   {
      it->second = uint_c(part[i]);
   }

   globalTimer.end();
   if (subComm != MPI_COMM_NULL)
   {
      int rank = -1;
      MPI_Comm_rank( subComm, &rank);
      if (rank == 0)  WALBERLA_LOG_INFO("ParMetis finished successfully after " << globalTimer.last() << " s (ParMetis took " << parmetisTimer.last() << " s = " << std::setprecision(2) << parmetisTimer.last() / globalTimer.last() * 100.0 << "%) with an edge - cut of " << edgecut );
   }

   MPI_Group_free(&allGroup);
   MPI_Group_free(&subGroup);
   //MPI_Comm_free(&subComm);

   return false; // no further iterations
}

DynamicParMetis::Algorithm DynamicParMetis::stringToAlgorithm( std::string s )
{
   boost::algorithm::to_upper( s );
   boost::algorithm::trim( s );

   if( s == "PART_GEOM_KWAY" )
      return PARMETIS_PART_GEOM_KWAY;
   else if( s == "PART_KWAY" )
      return PARMETIS_PART_KWAY;
   else if( s == "PART_ADAPTIVE_REPART" )
      return PARMETIS_ADAPTIVE_REPART;
   else if( s == "REFINE_KWAY" )
      return PARMETIS_REFINE_KWAY;
   else
      WALBERLA_ABORT( "Illegal ParMetis algorithm specified (" << s << ")! Valid choices are: \"PART_GEOM_KWAY\", \"PART_KWAY\", \"PART_ADAPTIVE_REPART\", or \"REFINE_KWAY\"." );
}


DynamicParMetis::WeightsToUse DynamicParMetis::stringToWeightsToUse( std::string s )
{
   boost::algorithm::to_upper( s );
   boost::algorithm::trim( s );

   if( s == "NO_WEIGHTS" )
      return PARMETIS_NO_WEIGHTS;
   else if( s == "EDGE_WEIGHTS" )
      return PARMETIS_EDGE_WEIGHTS;
   else if( s == "VERTEX_WEIGHTS" )
      return PARMETIS_VERTEX_WEIGHTS;
   else if( s == "BOTH_WEIGHTS" )
      return PARMETIS_BOTH_WEIGHTS;
   else
      WALBERLA_ABORT( "Illegal ParMetis weights usage specified (" << s << ")! Valid choices are: \"NO_WEIGHTS\", \"EDGE_WEIGHTS\", \"VERTEX_WEIGHTS\", or \"BOTH_WEIGHTS\"." );
}


DynamicParMetis::EdgeSource DynamicParMetis::stringToEdgeSource( std::string s )
{
   boost::algorithm::to_upper( s );
   boost::algorithm::trim( s );

   if( s == "EDGES_FROM_FOREST" )
      return PARMETIS_EDGES_FROM_FOREST;
   else if( s == "EDGES_FROM_EDGE_WEIGHTS" )
      return PARMETIS_EDGES_FROM_EDGE_WEIGHTS;
   else
      WALBERLA_ABORT( "Illegal ParMetis weights usage specified (" << s << ")! Valid choices are: \"EDGES_FROM_FOREST\" or \"EDGES_FROM_EDGE_WEIGHTS\"" );
}


std::string DynamicParMetis::algorithmToString( ) const
{
   switch (algorithm_)
   {
   case walberla::blockforest::DynamicParMetis::PARMETIS_PART_GEOM_KWAY:
      return "PART_GEOM_KWAY";
   case walberla::blockforest::DynamicParMetis::PARMETIS_PART_KWAY:
      return "PART_KWAY";
   case walberla::blockforest::DynamicParMetis::PARMETIS_ADAPTIVE_REPART:
      return "PART_ADAPTIVE_REPART";
   case walberla::blockforest::DynamicParMetis::PARMETIS_REFINE_KWAY:
      return "PARMETIS_REFINE_KWAY";
   }
   return "Unknown";
}


std::string DynamicParMetis::weightsToUseToString( ) const
{
   switch (weightsToUse_)
   {
   case walberla::blockforest::DynamicParMetis::PARMETIS_NO_WEIGHTS:
      return "NO_WEIGHTS";
   case walberla::blockforest::DynamicParMetis::PARMETIS_EDGE_WEIGHTS:
      return "EDGE_WEIGHTS";
   case walberla::blockforest::DynamicParMetis::PARMETIS_VERTEX_WEIGHTS:
      return "VERTEX_WEIGHTS";
   case walberla::blockforest::DynamicParMetis::PARMETIS_BOTH_WEIGHTS:
      return "BOTH_WEIGHTS";
   }
   return "Unknown";
}


std::string DynamicParMetis::edgeSourceToString( ) const
{
   switch (edgeSource_)
   {
   case walberla::blockforest::DynamicParMetis::PARMETIS_EDGES_FROM_FOREST:
      return "EDGES_FROM_FOREST";
   case walberla::blockforest::DynamicParMetis::PARMETIS_EDGES_FROM_EDGE_WEIGHTS:
      return "EDGES_FROM_EDGE_WEIGHTS";
   }
   return "Unknown";
}


} // namespace blockforest
} // namespace walberla
