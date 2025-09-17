#!/bin/bash

ARGS=$@
DB=`uuidgen | sed s/[\{\}]//g`.db
DB_SCHEMA_VERSION=`cat ../DB_SCHEMA_VERSION`
SHELL=sqlite3
TEST_DATA=test-data.sql

# Console Colors
ECHO_ESCAPE=-e

TEXT_FG_BLACK="[30m"
TEXT_FG_RED="[31m"
TEXT_FG_GREEN="[32m"
TEXT_FG_ORANGE="[33m"
TEXT_FG_BLUE="[34m"
TEXT_FG_MAGENTA="[35m"
TEXT_FG_CYAN="[36m"
TEXT_FG_GRAY="[37m"
TEXT_FG_DARK_GRAY="[30;1m"
TEXT_FG_LIGHT_RED="[31;1m"
TEXT_FG_LIGHT_GREEN="[32;1m"
TEXT_FG_YELLOW="[33;1m"
TEXT_FG_VIOLET="[34;1m"
TEXT_FG_LIGHT_MAGENTA="[35;1m"
TEXT_FG_LIGHT_CYAN="[36;1m"
TEXT_FG_WHITE="[37;1m"

TEXT_BG_BLACK="[40m"
TEXT_BG_RED="[41m"
TEXT_BG_GREEN="[42m"
TEXT_BG_YELLOW="[43m"
TEXT_BG_BLUE="[44m"
TEXT_BG_MAGENTA="[45m"
TEXT_BG_CYAN="[46m"
TEXT_BG_GRAY="[47m"

TEXT_NORM="[0m" # Back to normal text

function _errcho()
{
    echo "$@" 1>&2
}

function _print()
{
    _errcho $ECHO_ESCAPE $TEXT_FG_YELLOW"$1"$TEXT_NORM
}

function _error()
{
    _errcho $ECHO_ESCAPE $TEXT_BG_RED$TEXT_FG_YELLOW" ERROR: $1 "$TEXT_NORM
}

function _exec()
{
    _errcho $TEXT_FG_GREEN"$ "$@ $TEXT_NORM
    if ! $@
    then
        _error "Command execution failed."
        exit 1
    fi
}

function _banner()
{
    _errcho
    _errcho $ECHO_ESCAPE " "$TEXT_BG_CYAN$TEXT_FG_DARK_GRAY"┌──────────────"$TEXT_FG_WHITE"┐"$TEXT_NORM
    _errcho $ECHO_ESCAPE " "$TEXT_BG_CYAN$TEXT_FG_DARK_GRAY"│ "$TEXT_FG_YELLOW"TIM"$TEXT_FG_WHITE" Database"$TEXT_FG_LIGHT_WHITE" │"$TEXT_NORM
    _errcho $ECHO_ESCAPE " "$TEXT_BG_CYAN$TEXT_FG_DARK_GRAY"└"$TEXT_FG_WHITE"──────────────┘"$TEXT_NORM
    _errcho
}

_banner

_print "> Removing old databases..."
rm -f *.db *.db-shm *.db-wal
_print "> Done!"

_print "> Creating the database..."
for i in `ls ??-*.sql`
do
    _exec $SHELL -bail -batch -init $i -cmd .quit $DB
done
_print "> Done!"

if [ `expr match "$ARGS" '.*\brelease\b.*'` -eq "0" ]; then
    _print "> Adding test data..."
    _exec $SHELL -bail -batch -init $TEST_DATA -cmd .quit $DB
    _print "> Done!"
fi

# Set Schema Version
_print "> Setting the database schema version..."
echo "PRAGMA user_version = $DB_SCHEMA_VERSION;" | _exec $SHELL -bail -batch $DB
_print "> Done!"

_print "All Done!"
