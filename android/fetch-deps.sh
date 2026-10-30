#!/bin/sh
# Downloads the third-party sources the Android build compiles from (pinned versions).
# Run once before building: ./fetch-deps.sh
set -e
cd "$(dirname "$0")"
mkdir -p deps

fetch() # name url ref
{
    if [ -d "deps/$1/.git" ] && [ "$(git -C "deps/$1" rev-parse HEAD)" = "$4" ]; then
        echo "deps/$1 is up to date"
        return
    fi
    rm -rf "deps/$1"
    git init -q "deps/$1"
    git -C "deps/$1" fetch -q --depth 1 "$2" "$3"
    git -C "deps/$1" checkout -q FETCH_HEAD
    actual="$(git -C "deps/$1" rev-parse HEAD)"
    if [ "$actual" != "$4" ]; then
        echo "deps/$1: expected commit $4 but got $actual" >&2
        exit 1
    fi
    echo "fetched deps/$1 ($3)"
}

fetch SDL2       https://github.com/libsdl-org/SDL.git       release-2.30.9 c98c4fbff6d8f3016a3ce6685bf8f43433c3efcc
fetch SDL2_image https://github.com/libsdl-org/SDL_image.git release-2.8.2  abcf63aa71b4e3ac32120fa9870a6500ddcdcc89
fetch SDL2_mixer https://github.com/libsdl-org/SDL_mixer.git release-2.8.0  a37e09f85d321a13dfcf0d4432827ee09beeb623
fetch gl4es      https://github.com/ptitSeb/gl4es.git        c1d7d014fcf687ed33f30ce78b6480b3be1cbeee c1d7d014fcf687ed33f30ce78b6480b3be1cbeee
