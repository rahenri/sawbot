#!/bin/bash

make

time ./sawbot --max-depth 23  < samples/bench.in
