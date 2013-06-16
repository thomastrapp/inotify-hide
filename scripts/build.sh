#!/usr/bin/env bash

# build all builds:
#   build/debug/
#   build/release/
#   ...

set -o pipefail

red_f=$(tput setaf 1)
hl_f=$(tput setaf 5)
bold=$(tput bold)
hl_reset=$(tput op)
hl_reset_all=$(tput sgr0)

print_usage()
{
  echo "Usage:" "$0" "[path to project root]"
  echo "       Build all builds"
}

assert_preconditions()
{
  [ -d "$1" ] || {
    echo >&2 "Given parameter is not a directory"; exit 1;
  }

  # at least bash version 4 required (associative arrays)
  [ "4" -ge ${BASH_VERSION:0:1} ] || {
    echo >&2 \
      "Bash version >= 4 required, detected version ${BASH_VERSION:0:1}";
    exit 1;
  }
}

[ $# -eq 1 ] || { print_usage ; exit 0; }

assert_preconditions "$1"

declare -A builds
builds[release]="-DCMAKE_BUILD_TYPE=Release"
builds[debug]="-DCMAKE_BUILD_TYPE=Debug"
builds[scan_build]="-DSCAN_BUILD=ON"

cd "$1" || {
  echo >&2 "Failed changing working directory to $1"; exit 1; 
}

[ -d build ] || {
  mkdir build || { 
    echo >&2 "Build directory does not exist and mkdir failed"; exit 1; 
  }
}

cd build || {
  echo >&2 "Failed changing into build directory"; exit 1;
}

for build_type in "${!builds[@]}" ; do
  echo 
  echo "${hl_reset}${hl_f}*******************************${hl_reset}"
  echo "${hl_reset}${hl_f}***  ${bold}${red_f}$build_type${hl_reset_all}"
  echo "${hl_reset}${hl_f}*******************************${hl_reset}"

  [ -d "$build_type" ] || {
    mkdir "$build_type" || {
      echo >&2 "Directory $build_type doesn't exist and mkdir failed";
      exit 1;
    }
  }

  cd "$build_type" || {
    echo >&2 "Failed changing into directory $build_type"; exit 1;
  }

  make clean 2> /dev/null
  # there's no "cmake clean"
  find . -iname "*cmake*" -exec rm -rf {} \+

  cmake "${builds[$build_type]}" ../../ | sed "s/^/    /" || {
    echo >&2 "cmake ${builds[$build_type]} failed in build $build_type"; 
    exit 1;
  }

  # note the environment variable "CCC_CC=clang":
  # if it is not set to clang on my system, cmake will fall back to gcc
  if [ "$build_type" == "scan_build" ] ; then
    CCC_CC=clang scan-build make | sed "s/^/    /" || {
      echo >&2 "scan-build make failed in build $build_type"; 
      exit 1;
    }
  else
    make | sed "s/^/    /" || {
      echo >&2 "make failed in build $build_type"; 
      exit 1;
    }
  fi

  cd .. || {
    echo >&2 "Failed leaving build directory"; exit 1;
  }
done

cd ..
echo
find build/ -maxdepth 2 -executable -type f -exec ls -lah --color {} \;

