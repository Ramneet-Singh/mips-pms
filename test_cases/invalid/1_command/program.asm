ramneet 106
lw $t0, 9($zero
sw $44, 9($zero)
sw $t4, 1048577
add $t1, $t3, $t2, $t4
addi $t1
addi $t1, $t2, 3,
lw $zero, 8000
addi $zero, $t1, 2
lw $t1, 8001
j 8001
j 1048577

#The next three lines are a combined testcase
addi $t0, $zero, 2147483647
addi $t1, $zero, 2
add $t2, $t1, $t0

#The next three lines are a combined testcase
addi $t0, $zero, -2147483648
addi $t1, $zero, 1
sub $t2, $t0, $t1
