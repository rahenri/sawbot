#!/bin/bash

make

time ./sawbot --max-depth 20  < samples/bench.in
