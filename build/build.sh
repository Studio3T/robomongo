#!/bin/sh
set -e

function createPackage(){
    dir_path = "$1"
    cpack_generator = "$2"
    if [ -d "$dir_path" ]; then
      rm -rf "$dir_path"
    fi
    mkdir "$dir_path"
    cd "$dir_path"
    cmake ../../ -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=RELEASE -DCPACK_GENERATOR="$cpack_generator"
    make install
    cpack
    
    if ["$cpack_generator" -eq "DEB"]
        ./fix_up.sh
    fi
}
createPackage build_rpm RPM
createPackage build_deb DEB