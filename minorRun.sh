#!/bin/bash
for f in $(find assgn4-testcases/ -name '*.asm'); do 
	./assignment3 $f 10 2 0 > "$(echo "$f" | sed s/test.asm/output.txt/)"; 
done
for f in $(find assgn4-testcases/ -name '*.asm'); do 
	./assignment3 $f 10 2 1 > "$(echo "$f" | sed s/test.asm/output.txt/)"; 
done