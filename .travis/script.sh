#!/usr/bin/env bash

export LC_ALL=C.UTF-8

TRAVIS_COMMIT_LOG=$(git log --format=fuller -1)
export TRAVIS_COMMIT_LOG

DOCKER_EXEC ccache --max-size=$CCACHE_SIZE

mkdir build
cd build || (echo "could not enter build directory"; exit 1)

BEGIN_FOLD configure
DOCKER_EXEC cmake .. 
END_FOLD

BEGIN_FOLD build
DOCKER_EXEC make -k $MAKEJOBS
END_FOLD

BEGIN_FOLD ctest
DOCKER_EXEC ctest 
END_FOLD

if [ "$TRAVIS_EVENT_TYPE" = "cron" ]; then 
  extended="--extended --quiet --exclude pruning"
fi

