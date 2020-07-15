
.data
str1: .asciiz "Enter 3 integer numbers to show divisible numbers:\n"
terminate: .asciiz "ShowDivisibleNumbers finished.\n"
newline: .asciiz "\n"
divNumbs: .asciiz "Divisible numbers are:\n"
number1Print: .asciiz "Enter the first number:"
number2Print: .asciiz "Enter the second number:"
dividingPrint: .asciiz "Enter the dividing number:"
number1: .space 4
number2: .space 4
number3: .space 4

.text

.globl main
ShowDivisibleNumbers:
    #prints the str1
    li	$v0, 4
	la	$a0, str1
	syscall

    #prints the number1Print
    li	$v0, 4
	la	$a0, number1Print
	syscall
    # reads for number1
	li $v0, 5
	syscall
	move $t0, $v0

    #prints the number12rint
    li	$v0, 4
	la	$a0, number2Print
	syscall
    # reads for number2
    li $v0, 5
	syscall
	move $t1, $v0

    #prints the dividing
    li	$v0, 4
	la	$a0, dividingPrint
	syscall
    # reads for number3
    li $v0, 5
	syscall
	move $t2, $v0

    #runs the function
    move $a0,$t0
    move $a1,$t1
    move $a2,$t2
    jal mod_and_print
    jal exit

#a0:number1
#a1:number2
#a2:number3
mod_and_print:
    blt $a0,$a1,normalSet
reverseSet:
    move $s0,$a1 #gets the second number to $s0 register
    move $s1,$a0
    move $s2,$a2
    j afterSet
normalSet:
    move $s0,$a0 #gets the first number to $s0 register
    move $s1,$a1
    move $s2,$a2
afterSet:
    li	$v0, 4
	la	$a0, divNumbs
	syscall
loop:
    div $s0,$s2
    mfhi $t0
    bne $t0,$zero,jump #if $t0 and $zero equal then prints it.
    
    li	$v0, 1
	move $a0, $s0
	syscall

    li	$v0, 4
	la	$a0, newline
	syscall
jump:
    addi $s0,$s0,1
    slt $t1,$s1,$s0
    beq $t1,$zero,loop #if $t1 is not equal to $zero then goes to loop
    jr $ra


#exiting function
exit:
    #prints the bye
    li	$v0, 4
	la	$a0, terminate
	syscall
    
    li $v0, 10
   	syscall
