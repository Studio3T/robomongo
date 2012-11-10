# ****************
#  Configuration
# ****************

MODE=$1

if [ -z $MODE ]; then
   MODE=debug
fi

if [[ $MODE != "release" ]] && [[ $MODE != "debug" ]] && [[ $MODE != "all" ]]; then
    echo
    echo -e "\e[00;31m Specified mode ($MODE) is unsupported.\e[00m"
    echo
    exit 1
fi

# if 'all' specified - build in debug and release mode
if [[ $MODE = "all" ]]; then
  $0 debug
  $0 release
  exit 0
fi

PROJECT_ROOT=`pwd`/..                  # Find project root
TARGET=${PROJECT_ROOT}/target/$MODE    # Prepare output directory name
OUTPUT=${TARGET}/out                   # Prepare output directory name

if [[ $MODE = "debug" ]]; then
  QMAKE_ARGS="-r -spec linux-g++ CONFIG+=debug CONFIG+=declarative_debug"
fi
if [[ $MODE = "release" ]]; then
  QMAKE_ARGS="-r -spec linux-g++ CONFIG+=release"
fi

# ****************
# Preparation
# ****************

# Create target folder (if not already exists)
mkdir -p $TARGET
echo Output folder $TARGET

# Go to /target/debug folder
cd $TARGET

# ****************
# qmake
# ****************

qmake $PROJECT_ROOT/src/robomongo.pro $QMAKE_ARGS

if [ $? != 0 ]; then
{
    echo
    echo -e "\e[00;31m Error when running qmake.\e[00m"
    echo
    exit 1
} fi

# ****************
# make
# ****************

make -w

if [ $? != 0 ]; then
{
    echo 
    echo -e "\e[00;31m Error when running make.\e[00m"
    echo
    exit 1
} fi

echo
echo -e '\E[36m Done without errors \e[00m'
echo -e "\E[36m Executable location: ${OUTPUT} \e[00m"
echo