export TIM_PLATFORM=linux

_error()
{
    echo
    echo $ECHO_ESCAPE $TEXT_BG_RED$TEXT_FG_YELLOW" ERROR: $1 "$TEXT_NORM
    . build/$TIM_PLATFORM/failed.sh
    exit 1
}

_ex()
{
    echo $@
    if ! $@
    then
        . build/$TIM_PLATFORM/failed.sh
        exit 1
    fi
}
