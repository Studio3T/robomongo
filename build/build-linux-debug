# ****************
#  Configuration
# ****************

PROJECT_ROOT=`pwd`/.. # Find project root
ORIGINAL_DIR=`pwd`    # Store current directory in order to return to it after
TARGET=${PROJECT_ROOT}/target/debug # Prepare output directory name
OUTPUT=${TARGET}/out # Prepare output directory name

# ****************
# Preparation
# ****************

# Create target folder
mkdir -p $TARGET
echo Output folder $TARGET

# Go to /target/debug folder
cd $TARGET

# ****************
# qmake
# ****************

qmake $PROJECT_ROOT/src/robomongo.pro -r -spec linux-g++ CONFIG+=debug CONFIG+=declarative_debug

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