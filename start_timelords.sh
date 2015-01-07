#!/bin/bash
if [ ! `pgrep -n timelord_worker` ]; then for i in `seq 50`; do timelord_worker.sh& done; fi
if [ ! `pgrep -n timelord_router` ]; then timelord_router.sh& killall worker; fi
