
# For now the setup is hardcoded - down the line we might need to auto generate som

#determine top level directory from where setup script is located
if [ $# -gt 1 ];
then
  echo "Invalid number of arguments: $#"
  return 1
fi
if [ $# -eq 1 ]
then
  DAQLING_SPACK_REPO_PATH=$1
else
  DAQLING_SPACK_REPO_PATH=''
fi
pushd . > /dev/null
FASERTOP="${BASH_SOURCE[0]}"
if ([ -h "${FASERTOP}" ]); then
  while([ -h "${FASERTOP}" ]); do cd `dirname "$FASERTOP"`; 
  FASERTOP=`readlink "${FASERTOP}"`; done
fi
cd `dirname ${FASERTOP}` > /dev/null
FASERTOP=`pwd`;
popd  > /dev/null

echo "Setting up to run from ${FASERTOP}"
if [ $# -eq 1 ]
then
source ${FASERTOP}/daqling/cmake/setup.sh ${DAQLING_SPACK_REPO_PATH} ${FASERTOP}/configs/
else
source ${FASERTOP}/daqling/cmake/setup.sh
fi
#overwrite daqling setup variables to be director independent
export DAQ_SCRIPT_DIR=${FASERTOP}/daqling/scripts/
export DAQ_CONFIG_DIR=${FASERTOP}/configs/
export DAQ_BUILD_DIR=${FASERTOP}/build/
alias daqpy='python3 $FASERTOP/daqling/scripts/Control/daq.py'
alias rcgui='pushd $FASERTOP/scripts/Web; ./rcgui.py; popd'
alias rcguilocal='pushd $FASERTOP/scripts/Web; ./rcgui.py -l; popd'

#add python and binary directories needed for runnings
export PYTHONPATH=${FASERTOP}/daqling/scripts/Control:$PYTHONPATH
export PATH=${FASERTOP}/build/bin:${FASERTOP}/scripts/Web:${FASERTOP}/scripts/Monitoring:/home/cantel/root6/install/bin:$PATH
export LD_LIBRARY_PATH=/home/cantel/root6/install/lib:$LD_LIBRARY_PATH
