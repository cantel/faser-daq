# Usage: source setup.sh [config]
#
# where 'config' is using the standard syntax: ARCH-OS-COMPILER-BUILD
# e.g. x86_64-slc6-gcc62-opt
#

############## configuration variables ################
# You can override this in your environment for testing other versions.

# The standard AFS location of LCG software
#LCG_BASE=${LCG_BASE:=/afs/cern.ch/sw/lcg}
export LCG_BASE=${LCG_BASE:=/cvmfs/sft.cern.ch/lcg}
export LCG_VERSION=93
export LIBXKBCOMMON_VERSION=0.7.1

# The location and version of CMake to use
CMAKE_BASE=${CMAKE_BASE:=${LCG_BASE}/contrib/CMake}
CMAKE_VERSION=${CMAKE_VERSION:=3.4.3}
CMAKE_PATH=${CMAKE_PATH:=${CMAKE_BASE}/${CMAKE_VERSION}/Linux-x86_64/bin}

# The location of GCC, contrib seems to have more up to date compilers, external the debugger
CONTRIB_BASE=${CONTRIB_BASE:=${LCG_BASE}/contrib}
EXTERNAL_BASE=${EXTERNAL_BASE:=${LCG_BASE}/external}

# For TDAQ projects
#CMAKE_PROJECT_PATH=${CMAKE_PROJECT_PATH:=/afs/cern.ch/atlas/project/tdaq/cmake/projects}
# none on cvmfs for now...

#export JAVA_HOME=/afs/cern.ch/sw/lcg/external/Java/JDK/1.8.0/amd64
# none on cvmfs for now...

############## end of configuration variables ################
if [ -z "${1}" -o  "${1}" == "--" ]
then
    # no binary tag
    UNAME=`uname -r`
    case "${UNAME}" in
        *.el6.*)
            export BINARY_TAG="x86_64-slc6-gcc62-opt"
            ;;
        *)
            export BINARY_TAG="x86_64-centos7-gcc62-opt"
            ;;
    esac
else
    export BINARY_TAG=${1}
    shift
fi

echo "Setting up DAQ environment using BINARY_TAG: ${BINARY_TAG}"

if [ ! -d "${LCG_BASE}" ]; then
    echo "LCG_BASE Directory Not Found: ${LCG_BASE}"
    return 1
fi

# Setup path for CMAKE
export PATH=$(dirname $(readlink -f ${BASH_SOURCE[0]})):${CMAKE_PATH}:${PATH}

# determine OS
case "${BINARY_TAG}" in
    *-slc6-*)
        CMAKE_ARCH=x86_64-slc6
        ;;
    *-centos7-*)
        CMAKE_ARCH=x86_64-centos7
        ;;
    *)
        CMAKE_ARCH=x86_64-slc6
        ;;
esac

# Choose compiler, this has to reflect what you choose in the tag to
# init_build_area later, including the patch version !
case "${BINARY_TAG}" in
    *-slc6-gcc7-*)
        export LCG_VERSION=93
        export CMAKE_COMPILER=gcc7
        export BOOST_VERSION=1_66
        export PYTHON_VERSION=2.7.13
        source ${CONTRIB_BASE}/gcc/7/${CMAKE_ARCH}/setup.sh

        # debugger setup
        export PATH=${EXTERNAL_BASE}/gdb/7.6/${CMAKE_ARCH}-gcc48-opt/bin:${PATH}
        export LD_LIBRARY_PATH=${EXTERNAL_BASE}/gdb/7.6/${CMAKE_ARCH}-gcc48-opt/lib:${LD_LIBRARY_PATH}
        export LD_LIBRARY_PATH=${EXTERNAL_BASE}/gdb/7.6/${CMAKE_ARCH}-gcc48-opt/lib64:${LD_LIBRARY_PATH}
        ;;
    *-centos7-gcc7-*)
        export LCG_VERSION=93
        export CMAKE_COMPILER=gcc7
        export BOOST_VERSION=1_66
        export PYTHON_VERSION=2.7.13
        echoo ${CONTRIB_BASE}/gcc/7/${CMAKE_ARCH}/setup.sh
        source ${CONTRIB_BASE}/gcc/7/${CMAKE_ARCH}/setup.sh
        ;;
    *-*-icc180-*)
        export LCG_VERSION=93
        export CMAKE_COMPILER=gcc62
        export BOOST_VERSION=1_66
        export PYTHON_VERSION=2.7.13
        source ${CONTRIB_BASE}/gcc/6.2/${CMAKE_ARCH}/setup.sh
        export CC=icc
        export CXX=icpc
        ;;
    *)
        #default (gcc62)
        export LCG_VERSION=93
        export CMAKE_COMPILER=gcc62
        export BOOST_VERSION=1.66
        export BOOST_ROOT=${LCG_BASE}/releases/Boost/1.66.0-3ba23/${CMAKE_ARCH}-gcc62-opt
        export TBB_VERSION=2019.0
        export TBB_ROOT_DIR=${LCG_BASE}/releases/tbb/2019_U1-b3f06/${CMAKE_ARCH}-gcc62-opt
        export PYTHON_VERSION=2.7.13
        source ${CONTRIB_BASE}/gcc/6.2/${CMAKE_ARCH}/setup.sh
        ;;
esac

echo ${CONTRIB_BASE}/gcc/7/${CMAKE_ARCH}/setup.sh

# Setup jinja and pyyaml for wuppercodegen
export PYTHONPATH=$(dirname $(readlink -f ${BASH_SOURCE[0]}))/../../external/jinja/2.8:${PYTHONPATH}
export PYTHONPATH=$(dirname $(readlink -f ${BASH_SOURCE[0]}))/../../external/pyyaml/3.12:${PYTHONPATH}
export PYTHONPATH=$(dirname $(readlink -f ${BASH_SOURCE[0]}))/../../external/markupsafe/0.23:${PYTHONPATH}
export PATH=${LCG_BASE}/releases/LCG_${LCG_VERSION}/Python/${PYTHON_VERSION}/${BINARY_TAG}/bin:${PATH}

# Setup CMAKE_PREFIX_PATH to find LCG
# export CMAKE_PREFIX_PATH=${CMAKE_PROJECT_PATH}:$(dirname $(dirname $(dirname $(readlink -f ${BASH_SOURCE[0]}))))/cmaketools:$(dirname $(dirname $(readlink -f ${BASH_SOURCE[0]})))/cmake:${LCG_BASE}/releases
export CMAKE_PREFIX_PATH="${CMAKE_PROJECT_PATH} $(dirname $(dirname $(readlink -f ${BASH_SOURCE[0]})))/cmake ${LCG_BASE}/releases"

unset CMAKE_BASE CMAKE_VERSION CMAKE_PATH GCC_BASE CMAKE_ARCH
