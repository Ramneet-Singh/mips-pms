#!/bin/bash
for f in $(find assgn4-testcases/ -name '*.asm'); do 
	./assignment4 $f 10 2 1 > "$(echo "$f" | sed s/test.asm/output.txt/)"; 
done