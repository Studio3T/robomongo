
PROJECT_ROOT=`pwd`/..                  # Find project root
OUTPUT=${PROJECT_ROOT}/target/release/app/out  
TEMPLATE=${PROJECT_ROOT}/install/osx/Robomongo.app
CREATEDMG=${PROJECT_ROOT}/install/osx/yoursway-create-dmg/create-dmg


# add 'executable' bit to create-dmg if needed
if [ ! -x $CREATEDMG ] ; then
  chmod u+x $CREATEDMG  
fi

OS=`uname`

if [[ $OS = "Darwin" ]]; then

  cd $OUTPUT
  macdeployqt Robomongo.app
  cp -R $TEMPLATE $OUTPUT
  
  $CREATEDMG --window-size 500 300 --icon-size 96 --volname "Robomongo" --app-drop-link 325 125 --icon "Robomongo" 170 125 Robomongo.dmg $OUTPUT/Robomongo.app
  
  # example call to create-dmg
  # ./create-dmg --window-size 500 300 --background ~/Projects/eclipse-osx-repackager/build/background.gif --icon-size 96 --volname "Hyper Foo" 
  # --app-drop-link 380 205 --icon "Eclipse OS X Repackager" 110 205 test2.dmg /Users/andreyvit/Projects/eclipse-osx-repackager/temp/Eclipse\ OS\ X\ Repackager\ r10/
  
  echo "Robomongo.app location:"
  echo "$OUTPUT"
  
else
  echo "This script works only for Mac OS X"
fi


