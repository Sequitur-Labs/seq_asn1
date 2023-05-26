#!/bin/bash
# This script is for running indent on source files and writing the output
# to a destination directory.  The directory is the first argument, and
# the source files follow.

if [  $# -lt 1  ]; then
	echo "Usage: $0 <source-file-1> [<source-file-2> ...]"
	exit 1
fi

# Indent requires that all typedefs be defined so that pointers in
# declarations are properly handled.
TYPEDEFS="-T uint8_t -T uint32_t -T uint64_t -T int8_t -T int32_t -T int64_t -T size_t "

#For TEE Client
TYPEDEFS+="-T SeqDerIterator -T SeqDerNode -T SeqFreeTreeMode "

for i in $* ; do
	indent -linux -l132 -as -pmt $TYPEDEFS $i
done
