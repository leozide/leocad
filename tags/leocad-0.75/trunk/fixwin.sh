#!/bin/sh
# script to rename all files to lowercase and remove ^M from them

for j in linux common ; do
for i in $j/*.cpp $j/*.c $j/*.h $j/*.mk ; do
  if [ -f $i ]; then
  echo $i | awk \
  '{ 
    if (tolower($1) != $1)
	{
	    system ("mv "$1" "$1".lower")
	    system ("mv "$1".lower "tolower($1))
	    print $i
	}
    }'
  grep -ls 
 $i | perl -pi -e 's/
$//g' 
  fi
done
done
