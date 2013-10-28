#!/bin/sh
set -e

createPackage() {
    dir_path="$1"
    cpack_generator="$2"
    if [ -d "$dir_path" ]; then
        rm -rf "$dir_path"
    fi
    mkdir "$dir_path"
    cd "$dir_path"
    cmake ../../ -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=RELEASE -DCPACK_GENERATOR="$cpack_generator" -DOPENSSL_USE_STATIC=$3
    make install
    cpack -G "$cpack_generator"
    if [ "$cpack_generator" = 'DEB' ]; then
        sh ./fixup_deb.sh
    fi
    cd ../
}

unamestr=`uname`

if [ "$unamestr" = 'Linux' ]; then
    createPackage build_deb DEB 1
    createPackage build_rpm RPM 1
    createPackage build_tar TGZ 1
elif [ "$unamestr" = 'Darwin' ]; then
    createPackage build_dmg DragNDrop 0
    createPackage build_zip ZIP 0
fi

