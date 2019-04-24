export LD_LIBRARY_PATH=/opt/ohpc/pub/compiler/gcc/7.3.0/lib64/:$LD_LIBRARY_PATH

# Export package specific environmental variables

#gcc_home=/cvmfs/sft.cern.ch/lcg/releases/gcc/7.3.0-cb1ee/x86_64-centos7

#export FC=`which gfortran`
export CC='/opt/ohpc/pub/compiler/gcc/8.3.0/bin/gcc'
export CXX='/opt/ohpc/pub/compiler/gcc/8.3.0/bin/g++'

export TBB_VERSION=4.1
#export TBB_ROOT_DIR=/opt/tbb

#export COMPILER_PATH=${gcc_home}

