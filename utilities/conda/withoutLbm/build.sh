mkdir build
cd build
export BOOST_ROOT=$PREFIX

CC=mpicc CXX=mpicxx cmake .. -DWALBERLA_BUILD_WITH_PYTHON=1 -DWALBERLA_BUILD_WITH_PYTHON_MODULE=1

make -j 8 pythonModuleInstall
