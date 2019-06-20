#!/usr/bin/env bash

export LC_ALL=C.UTF-8

TRAVIS_COMMIT_LOG=$(git log --format=fuller -1)
export TRAVIS_COMMIT_LOG

OUTDIR=$BASE_OUTDIR/$TRAVIS_PULL_REQUEST/$TRAVIS_JOB_NUMBER-$HOST
BITCOIN_CONFIG_ALL="--with-seeder=false --disable-tests --prefix=$TRAVIS_BUILD_DIR/depends/$HOST --bindir=$OUTDIR/bin --libdir=$OUTDIR/lib"
if [ -z "$NO_DEPENDS" ]; then
  DOCKER_EXEC ccache --max-size=$CCACHE_SIZE
fi

BEGIN_FOLD autogen
if [[ $HOST = x86_64-linux-gnu ]]; then
  DOCKER_EXEC echo "skipping autogen for cmake build"
elif [ -n "$CONFIG_SHELL" ]; then
  DOCKER_EXEC "$CONFIG_SHELL" -c "./autogen.sh"
else
  DOCKER_EXEC ./autogen.sh
fi
END_FOLD

mkdir build
cd build || (echo "could not enter build directory"; exit 1)

BEGIN_FOLD configure
if [[ $HOST = x86_64-linux-gnu ]]; then
  DOCKER_EXEC cmake ..
else
  DOCKER_EXEC ../configure --cache-file=../config.cache $BITCOIN_CONFIG_ALL $BITCOIN_CONFIG || ( cat config.log && false)
fi
END_FOLD

BEGIN_FOLD distdir
if [[ $HOST = x86_64-linux-gnu ]]; then
  DOCKER_EXEC echo "skipping make distdir"
else
  DOCKER_EXEC make distdir VERSION=$HOST
fi
END_FOLD

if [[ $HOST = x86_64-linux-gnu ]]; then
  DOCKER_EXEC echo "no distdir"
else
  cd "devault-$HOST" || (echo "could not enter distdir devault-$HOST"; exit 1)
fi

BEGIN_FOLD configure
if [[ $HOST = x86_64-linux-gnu ]]; then
  DOCKER_EXEC echo "already configured with cmake"
else
  DOCKER_EXEC ./configure --cache-file=../config.cache $BITCOIN_CONFIG_ALL $BITCOIN_CONFIG || ( cat config.log && false)
fi
END_FOLD

BEGIN_FOLD build
if [[ $HOST = x86_64-linux-gnu ]]; then
  DOCKER_EXEC make $MAKEJOBS
else
  DOCKER_EXEC make $MAKEJOBS $GOAL || ( echo "Build failure. Verbose build follows." && DOCKER_EXEC make $GOAL V=1 ; false )
fi
END_FOLD

if [[ $HOST = x86_64-linux-gnu ]]; then
  DOCKER_EXEC make $MAKEJOBS check VERBOSE=1
fi

if [ "$TRAVIS_EVENT_TYPE" = "cron" ]; then 
  extended="--extended --quiet --exclude pruning"
fi

