l=`ls *.[hH] *.[cC] Examples/*.cc Examples/*.C Tests/*.cc Tests/*.H`
echo $l

for f in $l
do
    mv $f $f.tmp
    ./put-copyright $f.tmp > $f
done
	 
