#!/bin/sh

set -e

make -j4
cp sawbot sawbot-candidate 

TIME_PER_MOVE=100

BOT_CMD="./sawbot-candidate"

exec ./test/test.py "${BOT_CMD} --tee-input p1tee.txt" "${BOT_CMD} --tee-input p2tee.txt" --count 1 --verbose --workers 1 --logdir logs/ --time-per-move ${TIME_PER_MOVE} "$@"
