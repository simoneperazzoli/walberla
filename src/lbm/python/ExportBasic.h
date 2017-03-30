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
//! \file ExportBasic.h
//! \ingroup lbm
//! \author Martin Bauer <martin.bauer@fau.de>
//
//======================================================================================================================

#pragma once

#include "waLBerlaDefinitions.h"
#ifdef WALBERLA_BUILD_WITH_PYTHON

namespace walberla {
namespace lbm {

    void exportCollisionModels();
    void exportForceModels();

    template< typename LatticeModels > struct VelocityAdaptorsFromLatticeModels;
    template< typename LatticeModels > struct DensityAdaptorsFromLatticeModels;
    template< typename LatticeModels > struct AdaptorsFromLatticeModels;
    template< typename LatticeModels > struct ExtendedBoundaryHandlingsFromLatticeModels;

    template<typename LatticeModels, typename FlagFields>
    void exportBasic();

} // namespace lbm
} // namespace walberla


#include "ExportBasic.impl.h"

#endif // WALBERLA_BUILD_WITH_PYTHON
