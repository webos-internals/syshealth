#!/bin/sh
#
# Run VACUUM job on SQLite databases. This might take a while.
#

for i in `find / -name "*.db*" -print`
do

    file=`basename $i`
        
    if [ $file = Thumbs.db ]
    then
        continue
    else
    	echo "cleaning $i"
    	echo 'VACUUM;' | /usr/bin/sqlite3 $i
    fi

done

