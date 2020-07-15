.data
space: .asciiz " "
newline: .asciiz "\n"
terminate: .asciiz "Kernel finished.\n"
linearSearch: .asciiz "LinearSearch.asm"
binarySearch: .asciiz "BinarySearch.asm"
collatz: .asciiz "Collatz.asm"
rng: .asciiz "RNG:"
.text

.globl main
main:
    #Random Number Generator
    li $v0,24
    li $a0,3
    syscall
    move $t0,$v0

    li $t1,1
    li $t2,2
    beq $t0,$zero,linear_section  
    beq $t0,$t1,binary_section
    beq $t0,$t2,collatz_section


linear_section:
    #init syscall
    li $v0,22
    syscall

    #waitpid
    li $v0,20
    li $a0,0
    syscall

    li $t3,0
    li $t4,10
linear_loop:
    #fork syscall
    li $v0,18
    syscall
    
    #execve syscall
    li $v0,19
    la $a0,linearSearch
    syscall
    addi $t3,$t3,1
    bne $t4,$t3 linear_loop
    jal exitSpim

binary_section:
    #init syscall
    li $v0,22
    syscall

    #waitpid
    li $v0,20
    li $a0,0
    syscall

    li $t3,0
    li $t4,10
binary_loop:
    #fork syscall
    li $v0,18
    syscall
    
    #execve syscall
    li $v0,19
    la $a0,binarySearch
    syscall
    addi $t3,$t3,1
    bne $t4,$t3 binary_loop
    jal exitSpim

collatz_section:
    #init syscall
    li $v0,22
    syscall

    #waitpid
    li $v0,20
    li $a0,0
    syscall

    li $t3,0
    li $t4,10
collatz_loop:
    #fork syscall
    li $v0,18
    syscall
    
    #execve syscall
    li $v0,19
    la $a0,collatz
    syscall
    addi $t3,$t3,1
    bne $t4,$t3 collatz_loop
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
    
