l=`ls *.[hH] *.[cC] Examples/*.cc Examples/*.C Tests/*.cc Tests/*.H`

for f in $l
do
    echo $f
    mv $f $f.tmp
    ./put-copyright $f.tmp > $f
done
	 
