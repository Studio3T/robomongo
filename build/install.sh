
VERSION=0.7.1
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
  
elif [[ $OS = "Linux" ]]; then
  QMAKE_PATH=`which qmake`
  QT_BASE_BIN_DIR=$(dirname ${QMAKE_PATH})
  QT_BASE_LIB_DIR=$QT_BASE_BIN_DIR/../lib
  QT_BASE_PLUGINS_DIR=$QT_BASE_BIN_DIR/../plugins
  DEPLOY_DIR=$OUTPUT/robomongo-$VERSION
  LINUX_TEMPLATE=${PROJECT_ROOT}/install/linux
  
  echo $QT_BASE_LIB_DIR
  
  # create deploy folder (if not already exists)
  mkdir -p $DEPLOY_DIR
  mkdir -p $DEPLOY_DIR/lib
  mkdir -p $DEPLOY_DIR/lib/imageformats
  mkdir -p $DEPLOY_DIR/bin
  
  cp -T $OUTPUT/libqjson.so.0.7.1 $DEPLOY_DIR/lib/libqjson.so.0
  cp -T $OUTPUT/libqscintilla2.so.8.0.2 $DEPLOY_DIR/lib/libqscintilla2.so.8
  cp -T $QT_BASE_LIB_DIR/libQtCore.so.4.8.4 $DEPLOY_DIR/lib/libQtCore.so.4
  cp -T $QT_BASE_LIB_DIR/libQtGui.so.4.8.4 $DEPLOY_DIR/lib/libQtGui.so.4
  cp -T $QT_BASE_PLUGINS_DIR/imageformats/libqgif.so $DEPLOY_DIR/lib/imageformats/libqgif.so
  cp -T $OUTPUT/robomongo $DEPLOY_DIR/bin/robomongo
  cp -T $LINUX_TEMPLATE/robomongo.sh $DEPLOY_DIR/robomongo.sh
  cp -T $LINUX_TEMPLATE/README.txt $DEPLOY_DIR/README.txt
  
  cd $OUTPUT
  tar cvzf robomongo-$VERSION.tar.gz robomongo-$VERSION/
  
  echo "Linux... :) "
else 
  echo "This script works only for Mac OS X and Linux."
fi


