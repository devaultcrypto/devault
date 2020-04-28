#!/usr/bin/env bash

export LC_ALL=C.UTF-8

TRAVIS_COMMIT_LOG=$(git log --format=fuller -1)
export TRAVIS_COMMIT_LOG

OUTDIR=$BASE_OUTDIR/$TRAVIS_PULL_REQUEST/$TRAVIS_JOB_NUMBER-$HOST
if [ -z "$NO_DEPENDS" ]; then
  DOCKER_EXEC ccache --max-size=$CCACHE_SIZE
fi

mkdir build
cd build || (echo "could not enter build directory"; exit 1)

BEGIN_FOLD configure
if [[ $HOST = *-mingw32 ]]; then
    DOCKER_EXEC cmake -DCMAKE_TOOLCHAIN_FILE=$TRAVIS_BUILD_DIR/cmake/platforms/$CMAKE_OPT -DBUILD_SEEDER=OFF -DENABLE_REDUCE_EXPORTS=ON -DBUILD_QT=OFF -DCCACHE=OFF -DBUILD_STD_FILESYSTEM=OFF -DBUILD_CTESTS=OFF ..
else    
    DOCKER_EXEC cmake -DCMAKE_TOOLCHAIN_FILE=$TRAVIS_BUILD_DIR/cmake/platforms/$CMAKE_OPT -DBUILD_SEEDER=OFF -DENABLE_REDUCE_EXPORTS=ON -DBUILD_QT=OFF -DCCACHE=OFF -DBUILD_STD_FILESYSTEM=OFF ..
fi
END_FOLD

BEGIN_FOLD build
DOCKER_EXEC echo $pwd
DOCKER_EXEC cmake --build . 
END_FOLD

# just runs them if they are setup
BEGIN_FOLD CTests
if [[ $HOST = x86_64-linux-gnu ]]; then
    DOCKER_EXEC ctest 
fi
END_FOLD

if [ "$TRAVIS_EVENT_TYPE" = "cron" ]; then 
  extended="--extended --quiet --exclude pruning"
fi

