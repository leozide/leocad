#!/bin/sh
# $Id$

CC=$1
shift

# $CC -M -MG $* | \

( $CC -MM $* || exit 1 ) | \
sed \
	-e 's@^\(.*\)\.o:@\1.d \1.o:@'

