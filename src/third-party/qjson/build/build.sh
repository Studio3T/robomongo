# ****************
#  Configuration
# ****************

MODE=debug
while getopts ":dra" opt; do
  case $opt in
    d) MODE=debug ;;
    r) MODE=release ;;
    a) MODE=all ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      exit 1
      ;;
    :)
      echo "Option -$OPTARG requires an argument." >&2
      exit 1
      ;;
  esac
done

echo "Build mode: $MODE"

if [[ $MODE != "release" ]] && [[ $MODE != "debug" ]] && [[ $MODE != "all" ]]; then
    echo
    echo -e "\e[00;31m Specified mode ($MODE) is unsupported.\e[00m"
    echo
    exit 1
fi

# add 'executable' bit to clean.sh and rebuild.sh if needed
if [ ! -x `pwd`/"clean.sh" ] ; then
  chmod u+x clean.sh
  chmod u+x rebuild.sh
fi

# if 'all' specified - build in debug and release mode
if [[ $MODE = "all" ]]; then
  $0 -d
  $0 -r
  exit 0
fi

PROJECT_ROOT=`pwd`/..                  # Find project root
TARGET=${PROJECT_ROOT}/target/$MODE    # Prepare output directory name
OUTPUT=${TARGET}/out                   # Prepare output directory name

if [[ $MODE = "debug" ]]; then
  CMAKE_ARGS='-DCMAKE_BUILD_TYPE=Debug'
fi
if [[ $MODE = "release" ]]; then
  CMAKE_ARGS='-DCMAKE_BUILD_TYPE=Release'
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

cmake $CMAKE_ARGS $PROJECT_ROOT/src 

if [ $? != 0 ]; then
{
    echo
    echo -e "\e[00;31m Error when running cmake.\e[00m"
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