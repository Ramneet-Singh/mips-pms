lw $1, 0
lw $2, 1
lw $3, 2
lw $4, 3
sw $8, 4
sw $9, 5
addi $1, $1, 100
sw $12, 8
sw $10, 6
beq $4, $5, 20
bne $6, $1, 20
beq $4, $6, 20
bne $4, $6, 20
j 16
exit
add $4, $0, $1
add $5, $2, $3
sub $6, $2, $0
mul $7, $3, $1
add $8, $4, $5
sub $9, $1, $2
add $10, $9, $4
add $11, $10, $1
lw $12, 1028
lw $13, 1029
lw $13, 1030
j 15