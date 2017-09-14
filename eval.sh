#!/bin/sh

set -e
set -x

make -j4
cp sawbot sawbot-candidate 

exec ./test/test.py --config "config.json" --count 1000 --logdir logs/ --time-per-move 100 "$@"
