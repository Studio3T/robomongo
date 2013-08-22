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
    cmake ../../ -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=RELEASE -DCPACK_GENERATOR="$cpack_generator"
    make install
    cpack
    if ["$cpack_generator" -eq "DEB"]; then
        ./fixup_deb.sh
    fi
    cd ../
}

unamestr=`uname`

if [ "$unamestr"=='Linux' ]; then
    createPackage build_deb DEB
    createPackage build_rpm RPM
    createPackage build_tar TGZ
elif [ "$unamestr"=='Darwin' ]; then
    createPackage build_dmg DMG
    createPackage build_zip ZIP
fi

