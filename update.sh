#!/usr/bin/env bash

output=$(git fetch)

if [[ -n "$output" ]]; then
  $(git pull)
  $(make clean && ./configure && make)
  echo 'Updated'
else
  echo 'There is no update available'
fi
