#!/bin/bash
# Сборка бинарного .deb-пакета TIM для Ubuntu 24.04.
#
# Использование:
#   ./build-deb.sh         # сборка с тестами
#   ./build-deb.sh --nocheck  # без прогона тестов
#
# Зависимости:
#   apt install build-essential debhelper devscripts libssh-dev pkg-config
#
# Результат: ../tim_<version>_<arch>.deb в родительском каталоге проекта.

set -e

cd "$(dirname "$0")"

# Проверка минимально необходимых утилит.
missing=
for cmd in dpkg-buildpackage dh; do
    if ! command -v "$cmd" > /dev/null; then
        missing="$missing $cmd"
    fi
done
if [ -n "$missing" ]; then
    echo "ERROR: missing tools:$missing" >&2
    echo "Install with:" >&2
    echo "    sudo apt install build-essential debhelper devscripts libssh-dev pkg-config" >&2
    exit 1
fi

# Баннер с версией.
VERSION=$(tr -d '" \n' < VERSION)
echo
echo " ┌─────────────────────────────┐"
echo " │ TIM Debian package: $VERSION │"
echo " └─────────────────────────────┘"
echo

# Опции для dpkg-buildpackage:
#   -us  не подписывать .changes
#   -uc  не подписывать source.changes
#   -b   собирать только binary .deb (без source-пакета)
ARGS="-us -uc -b"
for a in "$@"; do
    case "$a" in
        --nocheck)
            export DEB_BUILD_OPTIONS="${DEB_BUILD_OPTIONS} nocheck"
            ;;
        *)
            ARGS="$ARGS $a"
            ;;
    esac
done

# Запуск сборки.
dpkg-buildpackage $ARGS

# Итог.
DEB=$(ls -1t ../tim_*.deb 2>/dev/null | head -1)
if [ -n "$DEB" ]; then
    echo
    echo "Package built: $DEB"
    ls -la "$DEB"
    echo
    echo "Inspect with:  dpkg-deb -I '$DEB'"
    echo "List files:    dpkg-deb -c '$DEB'"
    echo "Install:       sudo apt install '$DEB'"
else
    echo "WARN: no tim_*.deb found in $(dirname "$(pwd)")" >&2
    exit 1
fi
