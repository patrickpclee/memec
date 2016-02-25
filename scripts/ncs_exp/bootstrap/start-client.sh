#!/bin/bash

MASTER_NAME=$(hostname | sed 's/testbed-//g')
MASTER_IP=$(hostname -I | awk '{print $1}' | xargs)
MASTER_PORT=9112
CONFIG_PATH=bin/config/ncs_exp
MEMEC_PATH=~/mtyiu/memec

echo "Starting master [${MASTER_NAME}]..."

cd ${MEMEC_PATH}

if [ $# -gt 0 ]; then
	# Debug mode
	gdb bin/client -ex "r -v \
		-p ${CONFIG_PATH} \
		-o master ${MASTER_NAME} tcp://${MASTER_IP}:${MASTER_PORT}/"
else
	bin/client -v \
		-p ${CONFIG_PATH} \
		-o master ${MASTER_NAME} tcp://${MASTER_IP}:${MASTER_PORT}/
fi
