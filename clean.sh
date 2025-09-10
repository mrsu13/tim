#!/bin/sh

BUILD_BASE_DIR=build

export TIM_PLATFORM=`uname -s | sed -e 's/\(.*\)/\L\1/'`

. $BUILD_BASE_DIR/$TIM_PLATFORM/general.sh

make clean
