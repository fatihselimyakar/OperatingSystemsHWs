.data
colValue: .word 7

double_point: .asciiz ":"
space: .asciiz " "
newline: .asciiz "\n"
terminate: .asciiz "\nCollatz finished.\n"
.text

.globl main
Collatz:
    li $t5,1
    li $t6,25
main_loop:
    li $v0,1
    move $a0, $t5
	syscall

    li $v0,4
    la $a0, double_point
	syscall

    move $a0,$t5
    jal _collatz

    li $v0,4
    la $a0, newline
	syscall

    beq $t5,$t6,exit
    addi $t5,$t5,1
    j main_loop


#a0:collatz number
_collatz:
    move $t0,$a0
    li $t7,2
loopCollatz:
    div $t0,$t7#(r-l)/2
    mfhi $t1
    beq $t1,$zero,even
odd:
    li $t3,3
    mult $t0,$t3
    mflo $t0
    addi $t0,$t0,1

    li $v0, 1
	move $a0, $t0
	syscall

    li $v0, 4
	la $a0, space
	syscall

    li $t4,1
    beq $t4,$t0,exit_collatz
    j loopCollatz
even:
    div $t0,$t7
    mflo $t0

    li $v0, 1
	move $a0, $t0
	syscall

    li $v0, 4
	la $a0, space
	syscall

    li $t4,1
    beq $t4,$t0,exit_collatz
    j loopCollatz
exit_collatz:
    jr $ra


#exiting function
exit:
    #prints the bye
    li	$v0, 4
	la	$a0, terminate
	syscall

    li $v0, 23
   	syscall

    #li $v0, 10
   	#syscall
