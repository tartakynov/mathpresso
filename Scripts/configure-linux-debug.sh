#!/bin/sh
mkdir ../Build
cd ../Build

cmake .. -DCMAKE_BUILD_TYPE=Debug -G"Unix Makefiles"

cd ../Scripts
