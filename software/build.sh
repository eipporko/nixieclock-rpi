#!/bin/sh
gcc -Wall -o clock clock.c -lwiringPi
gcc -Wall rencoder.c -lwiringPi -o rencoder
