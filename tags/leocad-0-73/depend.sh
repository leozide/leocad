#!/bin/sh

DEP=$1
DIR=$2
CC=$3
shift 3

echo -n "$DEP $DIR/" > $DEP
if { !(eval $CC -MM $*) >> $DEP; }; then
    rm -f $DEP;
    exit 1;
fi

#sed -e 's@^\(.*\)\.o:@\1.d \1.o:@'

