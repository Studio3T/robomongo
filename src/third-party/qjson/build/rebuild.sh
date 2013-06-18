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

# clean and build
./clean.sh $@
./build.sh $@
