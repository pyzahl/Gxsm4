#!/bin/sh

grep class *.h | grep -v \; | grep -v include | grep -v @ | grep -v // | grep -v \* | sed 's/:/: /' | awk '{printf("%15s & %7s & %20s & %s %s & %s %s %s %s %s %s\\\\\n",$1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11);}'
