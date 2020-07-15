.data
space: .asciiz " "
newline: .asciiz "\n"
terminate: .asciiz "Kernel finished.\n"
linearSearch: .asciiz "LinearSearch.asm"
binarySearch: .asciiz "BinarySearch.asm"
collatz: .asciiz "Collatz.asm"
.text

.globl main
main:
    #init syscall
    li $v0,22
    syscall
    
    #waitpid syscall
    li $v0,20
    li $a0,0
    syscall

    #fork syscall
    li $v0,18
    syscall

    #execve syscall
    li $v0,19
    la $a0,collatz
    syscall
    move $t0,$v0

    #fork syscall
    li $v0,18
    syscall
    
    #execve syscall
    li $v0,19
    la $a0,linearSearch
    syscall

    #fork syscall
    li $v0,18
    syscall
    move $t0,$v0
    
    #execve syscall
    li $v0,19
    la $a0,binarySearch
    syscall
    
    jal exitSpim
    

#exiting function
exitSpim:
    #prints the bye
    li	$v0, 4
	la	$a0, terminate
	syscall

    #delete process syscall
    li $v0,23
    syscall

    #process exit syscall
    loop:
    li $v0,21
    syscall
    j loop

    

