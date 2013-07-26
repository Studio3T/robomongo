#!/bin/bash

ROBO_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

export LD_LIBRARY_PATH="$ROBO_DIR/../lib:$LD_LIBRARY_PATH"
export QT_PLUGIN_PATH="$ROBO_DIR/../lib:$QT_PLUGIN_PATH"

"$ROBO_DIR/robomongo"
