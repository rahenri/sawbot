#!/bin/bash

make

time ./sawbot --max-depth 24  < samples/bench.in
