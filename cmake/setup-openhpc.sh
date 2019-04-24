echo "Custom compiler, installed by Ansible from OHPC."

export LD_LIBRARY_PATH=/opt/ohpc/pub/compiler/gcc/8.3.0/lib64/:$LD_LIBRARY_PATH
export PATH=/opt/ohpc/pub/compiler/gcc/8.3.0/bin:$PATH

# Export package specific environmental variables

export CC='/opt/ohpc/pub/compiler/gcc/8.3.0/bin/gcc'
export CXX='/opt/ohpc/pub/compiler/gcc/8.3.0/bin/g++'

export TBB_VERSION=2019.0
export TBB_ROOT_DIR=/opt/tbb-2019_U5

export BOOST_VERSION=1.70
export BOOST_ROOT_DIR=/opt/boost

