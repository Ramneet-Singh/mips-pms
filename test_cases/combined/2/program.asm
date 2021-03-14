addi $10, $10, 10
addi $11, $10, 50
mul $2, $10, $11
sub $3, $2, $10
add $4, $3, $11
addi $2, $2, -1
bne $2, $zero, 20
beq $2, $zero, 32
j 40
mul $0, $t1, $t2
slt $t7, $10, $11