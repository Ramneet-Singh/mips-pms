addi $t0, $t0, 10
add $t1, $t0, $zero
addi $a0, $a0, 0

add $a0, $a0, $t1
addi $t1, $t1, -1
bne $t1, $zero, 12 

add $t0, $a0, 0
add $t1, $t0, $zero
addi $a0, $t0, 0

addi $a0, $a0, $t1
addi $t1, $t1, -1
bne $t1, $zero, 36

addi $t0, $a0, 0
add $t1, $t0, $zero
addi $a0, $t0, 0

add $a0, $a0, $t1
addi $t1, $t1, -1
bne $t1, $zero, 60

mul $a1, $a0, $t0
sub $a2, $a2, $a1

sw $a2, 10000
lw $k0, 10000
sw $k0, 10004
lw $k1, 10004
slt $v0, $t1, $k0