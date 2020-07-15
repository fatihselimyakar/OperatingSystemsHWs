.data
terminate: .asciiz "\nShell Terminated.\n"
welcome: .asciiz "Welcome To SpimOS shell!\n"
enter: .asciiz "\nEnter the command;\nFor ShowDivisibleNumber enter 1,\nFor LinearSearch enter 2,\nFor SelectionSort enter 3,\nFor BinarySearch enter 4,\nFor exit enter 0/-1:"
buffer: .space 50
.text

.globl main
main:
    #prints welcome string
    li $v0,4
    la $a0,welcome
    syscall
loop:
    #prints enter string
    li $v0,4
    la $a0,enter
    syscall

    #Gets the user choice
    li $v0,5
    syscall
    move $t0,$v0

    #If the coice exit then terminate the shell
    ble	$t0,$zero,exit

    #CreateProcess syscall
    li $v0,18
    move $a0,$t0
    syscall

    j loop 

#exiting function
exit:
    #prints the bye
    li	$v0, 4
	la	$a0, terminate
	syscall
    
    li $v0, 10
   	syscall
    
