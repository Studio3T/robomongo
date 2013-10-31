#!/bin/sh
set -e

deleteDir() {
dir_path="$1"
if [ -d "$dir_path" ]; then
      rm -rf "$dir_path"
    fi
}

unamestr=`uname`

deleteDir build_releases
