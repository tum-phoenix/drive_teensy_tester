[![Build Status](https://travis-ci.org/tum-phoenix/drive_teensy_tester.svg?branch=master)](https://travis-ci.org/tum-phoenix/drive_teensy_tester)

# Introduction

Test application for UAVCAN implementation for teensy consisting of 
* `main.cpp`: main file containing setup() and loop()
* `teensy_uavcan.cpp`: some useful functions for the teensy in combination with uavcan
* `parameter.hpp`: examples parameter server
* `publisher.hpp`: example publisher
* `subscriber.hpp`: example subscriber

## Install

Clone and run `git submodule update --init --remote`. Make sure `lib/libuavcan`
is on branch `teensy-driver` and updated to have the latest implementation of libuavcan.

## Build

In PlatformIO just hit the build button and magic happens....
