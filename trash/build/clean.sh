#!/bin/sh
set -e

deleteDir() {
dir_path="$1"
if [ -d "$dir_path" ]; then
      rm -rf "$dir_path"
    fi
}

unamestr=`uname`

if [ "$unamestr"=='Linux' ]; then
    deleteDir build_deb
    deleteDir build_rpm
    deleteDir build_tar
elif [ "$unamestr"=='Darwin' ]; then
    deleteDir build_dmg
    deleteDir build_zip
fi
