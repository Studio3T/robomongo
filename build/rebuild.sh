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

# clean and build
./clean.sh $MODE
./build.sh $MODE
