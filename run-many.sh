#!/bin/sh

set -e

make -j4
cp sawbot sawbot-candidate 

BOT_CMD="./sawbot-candidate"

exec ./test/test.py "${BOT_CMD}" "${BOT_CMD}" --count 10000  "$@"
