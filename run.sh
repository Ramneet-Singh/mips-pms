#!/bin/bash
for f in $(find testcases/ -name '*.asm'); do 
	./assignment3 $f 10 2 1 > "$(echo "$f" | sed s/test.asm/output.txt/)"; 
done