sw $t0, 2000
lw $t1, 2000
addi $t1, $t1, $zero
addi $s0, $zero, 2000
addi $s1, $zero, 2032
loop:
    lw $t1, 0($s0)
    addi $t1, $t1, 6
    addi $s0, $s0, 4
    sw $t1, 0($s0)
    bne $s0, $s1, loop
exit: