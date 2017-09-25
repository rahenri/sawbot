#!/bin/bash

VERSIONS=$(git tag | grep '^v')

for v in ${VERSIONS}; do
  echo "Compiling ${v}"
  git checkout "${v}"
  make clean
  make -j8
  cp "sawbot" "sawbot-${v}"
done

git checkout master
