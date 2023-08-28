# COMP2291 Assignment 1
Joshua Santos, 45203083

Simple program that uses multiple threads to perform DNS lookups to resolve webistes to IP addresses.

## Required Files
* th-lookup.c
* th-lookup.h
* util.c
* util.h
* queue.c
* queue.h
* .txt input files containing URLs, each separated by a new line. 

## How to Build
Open a terminal and change directories to the folder containing the files, then execute
```
make
```
in the terminal.

Run
```
make clean
```
to remove outputs created by the makefile.

## How to Run
In a terminal, execute
```
./th-lookup input1.txt input2.txt ... input9.txt output.txt
```
The program can take up to 9 input files, and 1 output file. The output file must always be the last argument.