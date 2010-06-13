#!/bin/bash

make -f Makefile.lkl
autoreconf -fi;
rm -Rf autom4te*.cache;
