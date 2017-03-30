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
//! \file BasicVoxelFileReader.h
//! \ingroup geometry
//! \author Christian Godenschwager <christian.godenschwager@fau.de>
//! \brief Declares class StructuredGeometryFileBasicReader that provides a low-level reader for waLBerla geometry files.
//!
//! This reader has no dependencies towards waLberla or even boost so it can be used by external
//! software to read or write geometry files.
//
//======================================================================================================================

#pragma once

#include <fstream>
#include <string>
#include <vector>


namespace walberla {
namespace geometry {

struct CellAABB;

/*******************************************************************************************************************//**
 *
 * \brief Provides a low level reader for waLBerla geometry files.
 *
 * \tparam T The underlying datatype that is stored in binary form in the geometry file
 *
 * \ingroup geometry
 *
 **********************************************************************************************************************/
template<typename T>
class BasicVoxelFileReader
{
public:
   BasicVoxelFileReader();
   BasicVoxelFileReader( const std::string & _filename);
   BasicVoxelFileReader( const std::string & _filename, size_t _xSize, size_t _ySize, size_t _zSize, T value = T() );
   BasicVoxelFileReader( const std::string & _filename, size_t _xSize, size_t _ySize, size_t _zSize, const T * values );
   ~BasicVoxelFileReader();

   void open  ( const std::string & _filename );
   void create( const std::string & _filename, size_t _xSize, size_t _ySize, size_t _zSize, T value = T() );
   void create( const std::string & _filename, size_t _xSize, size_t _ySize, size_t _zSize, const T * values );
   void close ();

   bool isOpen() const;
   const std::string & filename() const;
   size_t numCells() const;

   size_t xSize() const;
   size_t ySize() const;
   size_t zSize() const;

   void read ( const CellAABB & cellAABB,       std::vector<T> & data ) const;
   void write( const CellAABB & cellAABB, const std::vector<T> & data );

private:
   mutable std::fstream filestream_; ///< fstream object managing the opened file

   std::string filename_; ///< Filename of the geometry file currently opened.

   std::streampos dataBegin_; ///< Position in the stream where to raw data starts

   size_t xSize_; ///< Extend of the currently open geometry file in x direction
   size_t ySize_; ///< Extend of the currently open geometry file in y direction
   size_t zSize_; ///< Extend of the currently open geometry file in z direction

}; // class StructuredGeometryFileBasicReader


/*******************************************************************************************************************//**
 *
 * \brief Helper class to provide a cell based axis-aligned bounding box
 *
 * The AABB includes the cells with their x coordinate lying in [xBegin, xEnd], their y coordinate
 * lying in [yBegin, yEnd] and their z coordinate lying in [zBegin, zEnd].
 *
 * \ingroup geometry
 *
 **********************************************************************************************************************/
struct CellAABB
{
   inline CellAABB();
   inline CellAABB(size_t _xBegin, size_t _yBegin, size_t _zBegin,
                   size_t _xEnd,   size_t _yEnd,   size_t _zEnd);

   inline size_t numCells() const;
   inline size_t xSize() const;
   inline size_t ySize() const;
   inline size_t zSize() const;

   size_t xBegin; ///< The minimal x coordinate of all cells included in the AABB.
   size_t yBegin; ///< The minimal y coordinate of all cells included in the AABB.
   size_t zBegin; ///< The minimal z coordinate of all cells included in the AABB.

   size_t xEnd; ///< The maximal x coordinate of all cells included in the AABB.
   size_t yEnd; ///< The maximal y coordinate of all cells included in the AABB.
   size_t zEnd; ///< The maximal z coordinate of all cells included in the AABB.

}; // class CellAABB

} // namespace geometry
} // namespace walberla

#include "BasicVoxelFileReader.impl.h"

