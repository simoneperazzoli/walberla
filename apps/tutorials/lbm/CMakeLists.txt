waLBerla_link_files_to_builddir( *.prm )
waLBerla_link_files_to_builddir( *.png )

waLBerla_add_executable ( NAME 01_BasicLBM
                          FILES 01_BasicLBM.cpp
                          DEPENDS blockforest core field lbm geometry timeloop gui )
                                                                                             
waLBerla_add_executable ( NAME 02_BasicLBM_ExemplaryExtensions
                          FILES 02_BasicLBM_ExemplaryExtensions.cpp
                          DEPENDS blockforest core field lbm geometry timeloop gui )
                                                                                             
waLBerla_add_executable ( NAME 03_LBLidDrivenCavity 
                          FILES 03_LBLidDrivenCavity.cpp
                          DEPENDS blockforest boundary core field lbm stencil timeloop vtk )
                          
