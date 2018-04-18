#!/bin/bash

l=`ls *.[HC]`
for f in $l; do cp $f $f.tmp; sed -f dft-show.sed $f.tmp> $f; done
