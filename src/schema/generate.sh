#!/bin/bash
flatc --cpp message.fbs
rm -rf ../../python/quickVidViewer/fbs 
flatc -o ../../python/quickVidViewer/ --python message.fbs
