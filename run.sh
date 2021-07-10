#!/bin/bash
for f in $(find testcases/ -name '*.asm'); do 
	./mips_pms 1 $f 10 2 1 1> "$(echo "$f" | sed s/test.asm/output.txt/)"; 
done