#!/bin/bash

for i in logs/bot.stderr*; do rm $i; done

for i in history/games-*.txt; do rm $i; done
