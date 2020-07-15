.data
space: .asciiz " "
newline: .asciiz "\n"
terminate: .asciiz "Kernel finished.\n"
linearSearch: .asciiz "LinearSearch.asm"
binarySearch: .asciiz "BinarySearch.asm"
collatz: .asciiz "Collatz.asm"
rng: .asciiz "RNG numbers:"
.text

.globl main
main:
    li $v0,4
    la $a0,rng
    syscall

    li $v0,24
    li $a0,3
    syscall
    move $t0,$v0
    
    li $v0,24
    li $a0,3
    syscall
    move $t7,$v0
    
    li $t1,1
    li $t2,2

    bne $t0,$t7,change3
    bne $t7,$zero,change
    addi $t7,$t7,1
change:
    bne $t7,$t1,change2
    addi $t7,$t7,1
change2:
    bne $t7,$t2,change3
    li $t7,0
change3:

    li $v0,1
    move $a0,$t7
    syscall

    li $v0,1
    move $a0,$t0
    syscall

    #init syscall
    li $v0,22
    syscall

    #waitpid
    li $v0,20
    li $a0,0
    syscall

    bne $t0,$zero,jump 
    la $a0,linearSearch
    jal execute3times
jump:   
    bne $t0,$t1,jump2
    la $a0,binarySearch
    jal execute3times
jump2:
    bne $t0,$t2,jump3
    la $a0,collatz
    jal execute3times
jump3:
    bne $t7,$zero,jump4 
    la $a0,linearSearch
    jal execute3times
jump4:
    bne $t7,$t1,jump5
    la $a0,binarySearch
    jal execute3times
jump5:
    bne $t7,$t2,jump6
    la $a0,collatz
    jal execute3times
jump6:
    jal exitSpim


#a0:choosen program
execute3times:
    move $t5,$a0
    
    li $t3,0
    li $t4,3
loop:
    #fork syscall
    li $v0,18
    syscall
    
    #execve syscall
    li $v0,19
    move $a0,$t5
    syscall

    addi $t3,$t3,1
    bne $t4,$t3 loop
    jr $ra


#exiting function
exitSpim:
    #prints the bye
    li	$v0, 4
	la	$a0, terminate
	syscall

    #delete process syscall
    li $v0,23
    syscall
    
    #process exit loop
    inf_loop:
    li $v0,21
    syscall
    j inf_loop
