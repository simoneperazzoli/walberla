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
//! \file FieldAccessorXYZ.h
//! \ingroup cuda
//! \author Martin Bauer <martin.bauer@fau.de>
//
//======================================================================================================================

#pragma once


#include <cuda_runtime.h>

#include "core/DataTypes.h"

namespace walberla {
namespace cuda {


   template<typename T>
   class FieldAccessorXYZ
   {
   public:
      FieldAccessorXYZ( char * ptr, size_t xOffset, size_t yOffset, size_t zOffset, size_t fOffset )
            : ptr_(ptr), xOffset_(xOffset), yOffset_(yOffset), zOffset_(zOffset), fOffset_(fOffset)
      {}

      __device__ void set( uint3 blockIdx, uint3 threadIdx )
      {
         ptr_ += threadIdx.x * xOffset_ +
                 blockIdx.x  * yOffset_ +
                 blockIdx.y  * zOffset_ ;
      }

      __device__ T & get()       { return * (T*)(ptr_);                }
      __device__ T & get( int f) { return * (T*)(ptr_ + f * fOffset_); }


      __device__ T & getNeighbor( int cx, int cy, int cz ) const
      {
         return * (T*)( ptr_ + cx * (int)(xOffset_) +
                               cy * (int)(yOffset_) +
                               cz * (int)(zOffset_) );
      }

      __device__ T & getNeighbor( int cx, int cy, int cz, int cf )
      {
         return * (T*)( ptr_ + cx * (int)(xOffset_) +
                               cy * (int)(yOffset_) +
                               cz * (int)(zOffset_) +
                               cf * (int)(fOffset_) );
      }

   protected:
      char * ptr_;
      size_t xOffset_;
      size_t yOffset_;
      size_t zOffset_;
      size_t fOffset_;
   };


} // namespace cuda
} // namespace walberla


