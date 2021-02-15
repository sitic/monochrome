#!/bin/bash
flatc --cpp message.fbs
rm -rf ../../python/monochrome/fbs
flatc -o ../../python/monochrome/ --python message.fbs
