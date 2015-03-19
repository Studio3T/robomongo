#!/bin/bash

#ROBO_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

SOURCE="${BASH_SOURCE[0]}"
while [ -h "$SOURCE" ]; do # resolve $SOURCE until the file is no longer a symlink
  DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"
  SOURCE="$(readlink "$SOURCE")"
  [[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE" # if $SOURCE was a relative symlink, we need to resolve it relative to the path where the symlink file was located
done
ROBO_DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"

export LD_LIBRARY_PATH="$ROBO_DIR/../lib:$LD_LIBRARY_PATH"
export QT_PLUGIN_PATH="$ROBO_DIR/../lib:$QT_PLUGIN_PATH"
export QT_XKB_CONFIG_ROOT=/usr/share/X11/xkb
export XKB_DEFAULT_RULES=base

"$ROBO_DIR/robomongo"
