#!/bin/bash
if [ -d "./bin" ]; then
    rm -rf ./bin/*
    cd ./build
    cmake ..
    #make -j$(shell grep -c ^processor /proc/cpuinfo 2>/dev/null)
    make -j4
    cd ..
    cd ./bin
    mkdir Containers Recipes
    cd ..
    cp config.json ./bin
else 
    echo "Please run setup.sh first"
fi