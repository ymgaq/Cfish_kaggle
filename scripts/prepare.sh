#!/bin/bash

cp ../src/cfish ./
sed '/^\s*#/d' main_base.py > main.py
tar -czvf submission.tar.gz main.py cfish
ls -l submission.tar.gz