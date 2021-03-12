lw $at, 0
lw $v0, 1
lw $v1, 2
lw $a0, 3
sw $t0, 4
sw $t1, 5
sw $t4, 8
sw $t2, 6
beq $a0, $a1, 20
bne $a2, $at, 20
beq $a0, $a2, 20
bne $a0, $a3, 20
j 16
exit
add $t4, $a0, $1
add $t5, $s2, $3
sub $t6, $s2, $0
mul $t7, $a3, $1
add $t0, $a0, $5
sub $t1, $a1, $2
add $t2, $t1, $4
add $t3, $t2, $1
lw $t4, 1028
lw $t5, 1029
lw $t6, 1030
j 14