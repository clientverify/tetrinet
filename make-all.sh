#!/bin/bash

rm -f *.o
make KTEST=1 tetrinet
cp tetrinet tetrinet-ktest

rm -f *.o
make tetrinet 
cp tetrinet tetrinet-native

rm -f *.o
make tetrinet-server 

rm -f *.o
make KLEE=1 tetrinet
