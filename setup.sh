# For now the setup is hardcoded - down the line we might need to auto generate som

#determine top level directory from where setup script is located
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

source ${FASERTOP}/daqling/cmake/setup.sh

#overwrite daqling setup variables to be director independent
export DAQ_CONFIG_DIR=${FASERTOP}/configs/
export DAQ_BUILD_DIR=${FASERTOP}/build/
alias daqpy='python3 $FASERTOP/daqling/scripts/Control/daq.py'

#add python and binary directories needed for runnings
export PYTHONPATH=${FASERTOP}/daqling/scripts/Control:$PYTHONPATH
export PATH=${FASERTOP}/build/bin:${FASERTOP}/scripts/Web:${FASERTOP}/scripts/Monitoring:$PATH
