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

if [[ $MODE != "release" ]] && [[ $MODE != "debug" ]] && [[ $MODE != "all" ]]; then
    echo
    echo -e "\e[00;31m Specified mode ($MODE) is unsupported.\e[00m"
    echo
    exit 1
fi

if [[ $MODE = "all" ]]; then
  $0 -d
  $0 -r
  exit 0
fi

PROJECT_ROOT=`pwd`/..                  # Find project root
TARGET_ROOT=${PROJECT_ROOT}/target
TARGET=${PROJECT_ROOT}/target/$MODE    # Prepare output directory name

if [ ! -d "$TARGET" ]; then
    echo
    echo -e "\E[36m Already clean: ${TARGET} \e[00m"
    echo
    exit 0
fi

cd $TARGET_ROOT
rm -rf -- $MODE
if [ $? != 0 ]; then
{
    echo 
    echo -e "\e[00;31m Error when removing ${TARGET}.\e[00m"
    echo
    exit 1
} fi

if [ ! -d "$TARGET" ]; then
    echo
    echo -e '\E[36m Done without errors. \e[00m'
    echo -e "\E[36m This folder removed: ${TARGET} \e[00m"
    echo
else    
    echo 
    echo -e "\e[00;31m Error when removing files. Not all files were removed. \e[00m"
    echo
fi