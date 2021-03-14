#!/bin/bash
for f in $(find test_cases/ -name '*.asm'); do 
	./assignment3 $f > "$(echo "$f" | sed s/program.asm/output.txt/)"; 
done