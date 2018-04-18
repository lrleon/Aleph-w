#!/bin/bash

l=`ls *.[HC]`
for f in $l; do cp $f $f.tmp; sed -f bin-tree.sed $f.tmp> $f; done