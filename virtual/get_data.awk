#!/bin/sh

for x in "rand" "fifo" 
do
	for y in "sort" "scan" "focus" 
	do
		filename=data/$x$y.csv
		frames=2
		while [ $frames -le 100 ]; do
			./virtmem 100 $frames $x $y | awk '{ if($2 != "result") print $0; }' >> $filename
			let frames=frames+1
		done
	done
done
