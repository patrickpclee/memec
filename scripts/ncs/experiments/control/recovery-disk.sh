#!/bin/bash

BASE_PATH=${HOME}/mtyiu
MEMEC_PATH=${BASE_PATH}/memec

sizes='1000000000 2000000000 4000000000 8000000000 10000000000'

for s in $sizes; do
	for iter in {1..10}; do
		mkdir -p ${BASE_PATH}/results/recovery-disk/$s/$iter

		echo "Preparing for the experiments for size = $s..."

		screen -S manage -p 0 -X stuff "${BASE_PATH}/scripts/util/start.sh $1$(printf '\r')"
		for i in 2 5 6 7; do
			ssh testbed-node$i "screen -S server -p 0 -X stuff \"$(printf '\r\r')${BASE_PATH}/scripts/bootstrap/start-backup-server.sh ${1}$(printf '\r\r')\""
		done
		read -p "Press Enter when ready..." -t 30
		# sleep 30

		echo "Writing $s bytes of data to the system..."
		size=$(expr $s \/ 4)
		for n in 3 4 8 9; do
			ssh testbed-node$n "screen -S ycsb -p 0 -X stuff \"${BASE_PATH}/scripts/experiments/client/recovery.sh $size $(printf '\r')\"" &
		done

		pending=0
		echo "Start waiting at: $(date)..."
		for n in 3 4 8 9; do
			if [ $n == 3 ]; then
				read -p "Pending: ${pending} / 4" -t $(expr $s \/ 25000000)
			else
				read -p "Pending: ${pending} / 4" -t 60
			fi
			pending=$(expr $pending + 1)
		done
		echo "Done at: $(date)."

		ssh testbed-node1 "screen -S coordinator -p 0 -X stuff \"$(printf '\r\r')seal$(printf '\r\r')\""
		sleep 4

		# Flush parity chunks to disk
		for n in {11..23} {37..39}; do
			ssh testbed-node$n "screen -S server -p 0 -X stuff \"$(printf '\r\r')p2disk$(printf '\r\r')\"" &
		done
		sleep 10

		# Clear kernel buffer cache
		for n in {11..23} {37..39}; do
			ssh testbed-node$n "sudo sh -c 'echo 3 >/proc/sys/vm/drop_caches'"
		done

		n=$(expr $RANDOM % 13 + 11)
		echo "Killing node $n..."

		ssh testbed-node$n "screen -S server -p 0 -X stuff \"$(printf '\r\r')sync$(printf '\r\r')\""
		sleep 5
		ssh testbed-node$n "screen -S server -p 0 -X stuff \"$(printf '\r\r')memory$(printf '\r\r')\""
		sleep 1
		ssh testbed-node$n "killall -9 server; screen -S server -p 0 -X stuff \"$(printf '\r\r')\""

		# sleep 20
		read -p "Press Enter after recovery..." -t $(expr $s \/ 10000000)

		ssh testbed-node1 "screen -S coordinator -p 0 -X stuff \"$(printf '\r\r')log$(printf '\r\r')\""

		screen -S manage -p 0 -X stuff "$(printf '\r\r')"
		sleep 10

		scp testbed-node$n:${MEMEC_PATH}/memory.log ${BASE_PATH}/results/recovery-disk/$s/$iter/
		scp testbed-node1:${MEMEC_PATH}/coordinator.log ${BASE_PATH}/results/recovery-disk/$s/$iter/
	done
done
