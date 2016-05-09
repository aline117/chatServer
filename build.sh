#! /bin/sh

[ ! -d tmp/core ] && mkdir tmp/core
[ ! -d tmp/kernel ] && mkdir tmp/kernel
[ ! -d tmp/app ] && mkdir tmp/app
[ ! -d tmp/main ] && mkdir tmp/main

make

