#!/bin/sh
#
# Run VACUUM job on SQLite databases. This might take a while.
#

for i in `find / -name "*.db*" -print`
do
    echo "cleaning $i"
    echo 'VACUUM;' | /usr/bin/sqlite3 $i
done

