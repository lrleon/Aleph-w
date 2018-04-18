files=`ls *.[HC]`
for f in $files 
do
    sed -e s/"I ("/"assert("/g $f > $f.tmp
    mv $f.tmp $f
done
