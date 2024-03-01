#!/bin/bash
export FLATC=flatc
${FLATC} --cpp message.fbs
rm -rf ../python/monochrome/fbs
${FLATC} -o ../python/monochrome/ --python message.fbs --python-typing
