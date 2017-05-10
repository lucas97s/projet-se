#!/bin/sh

#
# Tests de detection de changement
#

TEST=$(basename $0 .sh)

TMP=/tmp/$TEST
LOG=$TEST.log
V=${VALGRIND}		# appeler avec la var. VALGRIND à "" ou "valgrind -q"

exec 2> $LOG
set -x

rm -f *.tmp

fail ()
{
    echo "==> Échec du test '$TEST' sur '$1'."
    echo "==> Log : '$LOG'."
    echo "==> Exit"
    exit 1
}
  
# test de changement

cat > script.tmp <<'EOF'
#!/bin/sh
    # script pour faire un changement à chaque fois
    F=f1.tmp
    if [ -f $F ]
    then echo "present" ; rm $F ; r=1
    else echo "absentx" ; touch $F ; r=0
    fi
    exit $r
EOF
chmod +x script.tmp

N=5
for i in $(seq 1 $N)
do
    echo absentx
    echo present
done > f3.tmp

rm -f f1.tmp
./detecter -i 1 -l $((N*2)) ./script.tmp > f2.tmp \
	&& cmp -s f2.tmp f3.tmp 		|| fail "present-absent"
exit 0
