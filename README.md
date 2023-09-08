# COMP2291 Assignment 1
Joshua Santos, 45203083

Simple program that uses multiple threads to perform DNS lookups to resolve websites to IP addresses.

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

When no longer needed, run
```
make clean
```
to remove outputs created by the makefile (.o files and executables).

## How to Run
In a terminal:
```
./th-lookup input1.txt input2.txt ... input9.txt output.txt
```
The program can take up to 10 input files, and 1 output file. The output file must always be the last argument.

## Modifying Queue Size and Number of Resolver Threads
Adjust the following defined constants in th-lookup.h:
* ```QUEUESIZE```: Maximum number of items in the queue at any given time.
* ```MAX_RESOLVER_THREADS```: Number of resolver threads to create.
By default, these are set to 100 and 10 respectively.