#!/bin/bash

## runso inject

GDB=${GDB:-/usr/bin/gdb}

function usage ()
{
    echo "Usage: `basename $0` inject       <pid>"
    echo "       `basename $0` start_server <pid>"
    echo "       `basename $0` stop_server  <pid>"
}

rs_operate=$1
rs_target_pid=$2

[   "$rs_operate" != "inject" -a        \
    "$rs_operate" != "start_server" -a  \
    "$rs_operate" != "stop_server" ] &&
{
    usage
    exit 1
}

[ -z "$rs_target_pid" ] &&
{
    usage
    exit 1
}

[ ! -r /proc/$rs_target_pid ] &&
{
    echo "Error: *** pid $rs_target_pid does not exist."
    exit 1
}

function inject ()
{
    grep librunso.so /proc/$1/maps 1>/dev/null && 
    {
        echo "warning: inject librunso.so already."
        return 0
    }
    
    local result=`$GDB --quiet -nx /proc/$1/exe $1 <<EOF 2>&1
call __libc_dlopen_mode("librunso.so", 1)
EOF`
    echo "$result" | egrep -A 1000 -e "^\(gdb\)" | egrep -B 1000 -e "^\(gdb\)"
}

function start_server ()
{
    local result=`$GDB --quiet -nx /proc/$1/exe $1 <<EOF 2>&1
call rs_start_server()
EOF`
    echo "$result" | egrep -A 1000 -e "^\(gdb\)" | egrep -B 1000 -e "^\(gdb\)"
}

function stop_server ()
{
    local result=`$GDB --quiet -nx /proc/$1/exe $1 <<EOF 2>&1
call rs_stop_server()
EOF`
    echo "$result" | egrep -A 1000 -e "^\(gdb\)" | egrep -B 1000 -e "^\(gdb\)"
}

$@
