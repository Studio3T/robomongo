#!/bin/sh
set -e

if [ -d "$dir_path" ]; then
      rm -rf build_rpm
fi

if [ -d "$dir_path" ]; then
      rm -rf build_deb
fi