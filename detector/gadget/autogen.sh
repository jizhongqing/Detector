#!/bin/sh
aclocal -I m4 --install
autoheader
libtoolize -i -c -f
autoconf
automake -a -c -f
