#!/bin/bash
if [ -d "bin" ]; then
    echo "clean the executable output directory"
    rm -rf bin/*
    echo "Done!"
fi

if [ -d "lib" ]; then 
    echo "clean the library output directory"
    rm -rf lib/*
    echo "Done!"
fi

if [ -d "build" ]; then
    echo "clean the build directory"
    rm -rf build/*
    echo "Done!"
fi