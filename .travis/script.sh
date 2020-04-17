#!/usr/bin/env bash

export LC_ALL=C.UTF-8

TRAVIS_COMMIT_LOG=$(git log --format=fuller -1)
export TRAVIS_COMMIT_LOG

# Start vars for gitian builder docker
green="\033[38;5;40m"
magenta="\033[38;5;200m"
cyan="\033[38;5;87m"
reset="\033[0m"
THISDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
masterApiEndpoint="https://api.github.com"
repo="https://github.com/devaultcrypto/devault"
branch_or_tag="develop"
#End vars for gitian builder docker

OUTDIR=$BASE_OUTDIR/$TRAVIS_PULL_REQUEST/$TRAVIS_JOB_NUMBER-$HOST
BITCOIN_CONFIG_ALL="--with-seeder=false --disable-tests --prefix=$TRAVIS_BUILD_DIR/depends/$HOST --bindir=$OUTDIR/bin --libdir=$OUTDIR/lib"
if [ -z "$NO_DEPENDS" ]; then
  DOCKER_EXEC ccache --max-size=$CCACHE_SIZE
fi
if [ -z "$BUILD_GITIAN" ]
then
      echo "\$var is empty"
      mkdir build
      cd build || (echo "could not enter build directory"; exit 1)

      BEGIN_FOLD configure
      DOCKER_EXEC cmake .. 
      END_FOLD

      BEGIN_FOLD build
      DOCKER_EXEC make -k $MAKEJOBS
      END_FOLD

      BEGIN_FOLD test
      DOCKER_EXEC ctest 
      END_FOLD

      if [ "$TRAVIS_EVENT_TYPE" = "cron" ]; then 
          extended="--extended --quiet --exclude pruning"
      fi

else
      # Run gitian builder code
      git clone https://github.com/akshaynexus/docker-based-gitian-builder
      git clone $repo
      cd docker-based-gitian-builder
      bash ./build_builder.sh # installs the base virtual machine (ubuntu 18 bionic) and dependencies, takes 5-10 minutes
      sdate=`date +%s`
      echo -e "starting $BUILD_OS build of tag: ${branch_or_tag} at: `date`${reset}"
      time docker run -h builder --name builder-$sdate \
      -v $THISDIR/cache:/shared/cache:Z \
      -v $THISDIR/result:/shared/result:Z \
      builder \
    "${branch_or_tag}" \
    "${repo}" \
    "/shared/devault/contrib/gitian-descriptors/gitian-${BUILD_OS}.yml"
fi
