#!/bin/bash
while [ 1 ]; do
  build/router
  killall worker
  echo "restarting router and workers" | logger
done
