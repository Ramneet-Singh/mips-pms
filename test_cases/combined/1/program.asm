j 4;
  	lw $at,  	0x3E8($t0)     



lw $v0, 1004
lw $v1, 1008
lw $a0, 1012
addi $t0, $t0, 101
sw $t0, 	960
lw $t4, 960

sw      $t1,     500
sw 		$t4,      800
sw $t2,    600
beq $a0, $a1, 96
beq $a0, $a2, 80
addi $t1, $t1, 10
mul $t7, $at, $t1
slt $s7, $v0, $at



j 				64
add 			$t4, $a0, $t1
add $t5,    	$s2, $t3
sub $t6, $s2, $t0
add $t0, $a0, $t5
addi $a1, $a1, 5
sub $t1, $a1, $t2
add $t2, $t1, $t4
add $t3, $t2, $t1
lw $t4, 1028
lw $t5, 1032
lw $t6, 1036