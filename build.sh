#!/bin/sh

BUILD_BASE_DIR=build

export TIM_DEBUG_MODE=1
export TIM_SILENT=1

for i in "$@" ; do
    case $i in

        "release"|"-release"|"--release")
            unset TIM_DEBUG_MODE
        ;;

        "verbose"|"-verbose"|"--verbose")
            unset TIM_SILENT
        ;;

        "single"|"-single"|"--single")
            TIM_SINGLE_MAKE=1
        ;;

        *)
            echo "ERROR: Unknown command line option '$i'."
            exit 1
        ;;
esac
done

export TIM_PLATFORM=`uname -s | sed -e 's/\(.*\)/\L\1/'`

TIM_BUILD_MODE_HR=\
$([ -z "$TIM_DEBUG_MODE" ] && echo "release" || echo "debug")\
' '\
$([ -z "$TIM_SILENT" ] && echo "verbose" || echo "silent")\
' '\
$([ -z "$TIM_SINGLE_MAKE" ] && echo "multi-make" || echo "single-make")

TIM_JOBS_NUM=`cat /proc/cpuinfo | grep -i processor | wc -l`

clear

. $BUILD_BASE_DIR/$TIM_PLATFORM/colors
. $BUILD_BASE_DIR/$TIM_PLATFORM/banner.sh

echo " Platform ......... $TIM_PLATFORM"
echo " Build mode ....... $TIM_BUILD_MODE_HR"
echo " Number of jobs ... $TIM_JOBS_NUM"

. $BUILD_BASE_DIR/$TIM_PLATFORM/started.sh
. $BUILD_BASE_DIR/$TIM_PLATFORM/general.sh

if [ -z "$TIM_SINGLE_MAKE" ]; then
    _ex make -j $TIM_JOBS_NUM
else
    _ex make
fi

. $BUILD_BASE_DIR/$TIM_PLATFORM/completed.sh
