#!/bin/sh

set -e

make -j4
cp sawbot sawbot-candidate 

exec ./test/test.py --action train --config config.json --logdir logs --count 5000
