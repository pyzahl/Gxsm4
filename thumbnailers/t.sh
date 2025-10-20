#!/bin/bash

echo
for file in $@;
do
    if nctopng $file x.png; then
	echo "OK:  $file"
    else
	echo
	echo "EE:  $file *** Details *** ==>"
	nctopng -v $file x.png
        echo "******************"
    fi
done
