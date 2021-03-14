addi $t0, $t0, 10
addi $t1, $t1, 10
addi $t2, $t2, 100

beq $t0, $t1, 20
addi $t3, $t3, 10000
add $t3, $t0, $t1

bne $t1, $t2, 32
addi $t4, $t4, 100
add $t4, $t2, $t1

slt $v0, $t3, $t4