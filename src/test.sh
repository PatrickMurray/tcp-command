#! /usr/bin/env bash

make

./tcp

./tcp source_dne
./tcp source_dne target_dne
./tcp source_dne target_dne something_else

./tcp README
./tcp README readme_copy

if [ -d  testdir ]; then
	rm -rf testdir
fi

mkdir testdir

./tcp README testdir
./tcp README directory_dne/

./tcp README testdir/README
./tcp README testdir/readme_copy

./tcp testdir/README .

cd ..
./tcp/tcp tcp/README ../../Homework/HW2/tcp
