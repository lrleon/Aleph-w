#!/bin/bash

cd doc-tmp/

list=`ls *.[hcHC]`

for f in $list
do
    name=`basename $f`
    echo $f
    mv $f $f.tmp
    cat ../COPYRIGHT $f.tmp > $f 
    rm $f.tmp
done

