#!/bin/bash
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
 
#on creer un boucle infinie pour ensuite envoyer un signal au processus apres 10 secondes

(sleep 10 && killall -9 ./detecter || exit 1) &

#on utilise une option inexistante 
./detecter -x -i 1 echo toto > /dev/null

exit 0
