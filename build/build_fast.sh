#!/bin/sh
set -e

createPackage() {
    cpack_generator="$1"
    cmake ../../ -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=RELEASE -DCPACK_GENERATOR="$cpack_generator" -DOPENSSL_USE_STATIC=1
#   make install
    cpack -G "$cpack_generator"
    if [ "$cpack_generator" = 'DEB' ]; then
        sh ./fixup_deb.sh
    fi
}

unamestr=`uname`
dir_path=build_releases
if [ -d "$dir_path" ]; then
    rm -rf "$dir_path"
fi
mkdir "$dir_path"
cd "$dir_path"

if [ "$unamestr" = 'Linux' ]; then
    createPackage DEB
    createPackage RPM
    createPackage TGZ
elif [ "$unamestr" = 'Darwin' ]; then
    createPackage DragNDrop
    createPackage ZIP
fi

