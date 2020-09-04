#!/usr/bin/env bash

export LC_ALL=C.UTF-8

DOCKER_EXEC echo \> \$HOME/.devault

mkdir -p depends/SDKs depends/sdk-sources

if [[ $HOST = x86_64-apple-darwin14 ]]; then
  DOCKER_EXEC apt-get update
  DOCKER_EXEC apt-get install apt-transport-https ca-certificates gnupg software-properties-common wget -y
  DOCKER_EXEC wget https://apt.kitware.com/keys/kitware-archive-latest.asc && DOCKER_EXEC apt-key add kitware-archive-latest.asc
  DOCKER_EXEC apt-add-repository https://apt.kitware.com/ubuntu/ -y
  DOCKER_EXEC apt-get install cmake -y
fi

if [ -n "$OSX_SDK" -a ! -f depends/sdk-sources/MacOSX${OSX_SDK}.sdk.tar.gz ]; then
  curl --location --fail $SDK_URL/MacOSX${OSX_SDK}.sdk.tar.gz -o depends/sdk-sources/MacOSX${OSX_SDK}.sdk.tar.gz
fi
if [ -n "$OSX_SDK" -a -f depends/sdk-sources/MacOSX${OSX_SDK}.sdk.tar.gz ]; then
  tar -C depends/SDKs -xf depends/sdk-sources/MacOSX${OSX_SDK}.sdk.tar.gz
fi
if [[ $HOST = *-mingw32 ]]; then
  DOCKER_EXEC update-alternatives --set $HOST-g++ /usr/bin/$HOST-g++-posix
  DOCKER_EXEC update-alternatives --set $HOST-gcc /usr/bin/$HOST-gcc-posix
fi
if [ -z "$NO_DEPENDS" ]; then
  DOCKER_EXEC CONFIG_SHELL= make $MAKEJOBS -C depends HOST=$HOST $DEP_OPTS
fi

