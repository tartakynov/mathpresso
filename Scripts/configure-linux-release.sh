#!/bin/sh
mkdir ../Build
cd ../Build

cmake .. -DCMAKE_BUILD_TYPE=Release -G"Unix Makefiles"

cd ../Scripts
