#!/bin/sh
# Сборка и запуск тестов TIM.
# Использование:
#   ./build.sh         # debug-сборка и запуск
#   ./build.sh release # релиз-сборка и запуск
set -e

cd "$(dirname "$0")"

MODE=${1:-debug}
CXX=${CXX:-g++}

if [ "$MODE" = "release" ]; then
    OPT="-O2"
else
    OPT="-g -O0"
fi

INCLUDES="-I../src -I../src/mqtt -I../src/types -I../src/signal"

$CXX -std=c++17 -Wall -Wextra -Werror $OPT $INCLUDES \
    -o tim_test \
    test_main.cpp \
    test_mqtt_topic.cpp \
    ../src/mqtt/tim_mqtt_topic.cpp

echo "----"
./tim_test
