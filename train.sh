#!/bin/sh

set -e

while true; do

  make -j4

  cp sawbot sawbot-candidate 

  ./test/test.py --config config.json --count 16

  make data

  ./learning/train.py history/*.csv.gz

done
