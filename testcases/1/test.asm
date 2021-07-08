addi $s0, $s0, 1000
addi $s1, $s1, 2000
addi $s2, $zero, 40
addi $s3, $zero, 0
loop:
    lw $t0, 0($s0)
    lw $t1, 0($s1)
    addi $s0, $s0, 4
    addi $s1, $s1, 4
    addi $s3, $s3, 1
    bne $s3, $s2, loop
exit: