#!/bin/bash

make clean
make build -j ARCH=x86-64-bmi2 EXTRACFLAGS="-ffunction-sections -fdata-sections" EXTRALDFLAGS="-Wl,--gc-sections"
strip cfish
upx --lzma cfish
tar -zvcf cfish.tar.gz cfish
ls -l cfish*