#!/bin/bash
while [ 1 ]; do
  build/worker
  echo "restarting worker" | logger
done
