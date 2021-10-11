#!/bin/sh

DEFAULT_RHOSTNAME=rpi400
RHOSTNAME=${1}
RHOSTNAME=${DEFAULT_RHOSTNAME}
DELAY=2

# if [ "${RHOSTNAME}" == "" ]
# then
#   echo "Remote host not specified, using default:" ${DEFAULT_RHOSTNAME}
#   RHOSTNAME=${DEFAULT_RHOSTNAME}
# fi

echo "Remote host:" ${RHOSTNAME}
cd ..

while [ 1 ]
do
    rsync -rlptzv --progress --exclude=.git --exclude=.vscode *.c *.h *.cmake *.mk pi@${RHOSTNAME}:~/pico/modules/usqlite
    echo "Next update of ${RHOSTNAME} in ${DELAY} seconds"
    sleep ${DELAY}
done
