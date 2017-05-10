#!/bin/sh

TEST=$(basename $0 .sh)

TMP=/tmp/$TEST
LOG=$TEST.log
V=${VALGRIND}		# appeler avec la var. VALGRIND à "" ou "valgrind -q"
exec 2> $LOG
set -x

fail ()
{
    echo "==> Échec du test '$TEST' sur '$1'."
    echo "==> Log : '$LOG'."
    echo "==> Exit"
    exit 1
}

( ./detecter -c tail -f /dev/null )& 

PID=$( ps -ux |grep "./detecter -c tail -f /dev/null" |head -n1 |tr -s ' ' |cut -d' ' -f2 || fail "PID" ) 
PID=$( pgrep -P $PID )
kill $PID || fail "kill"
pgrep $PID || echo exited > $LOG
exit 0
