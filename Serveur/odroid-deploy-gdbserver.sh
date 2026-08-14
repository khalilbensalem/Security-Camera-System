#!/bin/bash
readonly TARGET_IP="192.168.7.2"
readonly TARGET_PORT="3000"
readonly PROGRAM="prog"
readonly PROGRAM_PATH="build/prog"
readonly TARGET_DIR="/home/root"

echo "Deploying on Target"

ssh root@${TARGET_IP} "sh -c '/usr/bin/killall -q gdbserver; rm -f ${TARGET_DIR}/${PROGRAM}; exit 0'"

scp ${PROGRAM_PATH} root@${TARGET_IP}:${TARGET_DIR}

echo "Starting gdbserver on Target"

ssh -t root@${TARGET_IP} "sh -c 'cd ${TARGET_DIR}; gdbserver localhost:${TARGET_PORT} ./${PROGRAM}'"