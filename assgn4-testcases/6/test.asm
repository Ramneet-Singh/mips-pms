main:
	addi $s0, $zero, 10
	addi $s1, $zero, 0
	addi $t1, $zero, 100

loop:
	lw $t0, 600($s1)
	sw $t1, 2100($s1)
	lw $t2, 5200($s1)
	addi $t1, $t1, -1
	addi $s1, $s1, 4
	addi $s0, $s0, -1
	bne $s0, $zero, loop
	
exit:
