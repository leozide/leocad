#!/bin/sh

DEP=$1
DIR=$2
CC=$3
shift 3

echo -n "$DEP $DIR/" > $DEP
( $CC -MM $* || exit 1 ) >> $DEP

#sed -e 's@^\(.*\)\.o:@\1.d \1.o:@'

