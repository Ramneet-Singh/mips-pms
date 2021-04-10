start:
	lw $t1, 2004
	sw $t1, 9000
	addi $s0, $zero 100
	lw $t0, 1004
	sw $t2, 1008
	lw $t3, 1012
	sw $t4, 2000
	sw $t5, 1000
	addi $s1, $zero 100
	slt $t2, $s0, $s1
	mul $t6, $s0, $s1
	sub $t7, $zero, $t6
	j next
next:
	bne $s1, $s0, start