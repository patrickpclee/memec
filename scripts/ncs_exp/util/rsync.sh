#!/bin/bash

for i in {1..10} {11..23} {37..39}; do
	rsync \
		--delete \
		--force \
		--progress \
		--verbose \
		--archive \
		~/mtyiu/plio/ testbed-node$i:mtyiu/plio/
done
