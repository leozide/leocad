#!/bin/sh

DEP=$1
CC=$2
shift 2

echo -n "$DEP" | awk '{ ORS = ""; print $1 " "; sub (".d", ".o"); print $1 " "; }' > $DEP
( $CC -MM $* || exit 1 ) >> $DEP

#sed -e 's@^\(.*\)\.o:@\1.d \1.o:@'

