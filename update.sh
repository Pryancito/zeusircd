#!/usr/bin/env bash

output=$(git fetch)

if [[ -n "$output" ]]; then
  $(git pull)
  $(make clean)
  $(./configure)
  $(make)
  echo 'ZeusiRCd has been updated'
else
  echo 'There is no update available'
fi
