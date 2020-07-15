.data
array: .word 60 20 22 75 50 51 10 1 0
size: .word 10 #ATTENTION : normal size+1

terminate: .asciiz "\nSelection sort finished.\n"
space: .asciiz " "
newline: .asciiz "\n"
printBefore: .asciiz "\nBefore the sort\n"
printAfter: .asciiz "\nAfter the sort\n"
sortedPrint: .asciiz "List is:"
.text

.globl main
SelectionSort:
    #String print
    la $a0,printBefore
    li $v0,4
    syscall

    #Array print
    la $a0,array
    li $a1,9
    jal print_array

    #Selection sort parameters and function
    la $a0,array
    lw $a1,size
    jal selection_sort

    #String print
    la $a0,printAfter
    li $v0,4
    syscall

    #Array print
    la $a0,array
    li $a1,9
    jal print_array

    jal exit 

#a0:array
#a1:size
selection_sort:
    li $t0,0 # $t0=0 (i=0)
    move $s2,$a0 #array
    move $s3,$a1 #size
    outer_loop:
        move $t1,$t0 # (min_index=i)
        addi $t2,$t0,1 # j=i+1
        inner_loop:
            li $t3,4
            mult $t1,$t3
            mflo $t4 
            add $t4,$t4,$s2
            lw $s0,0($t4)#arr[min_index]
            
            mult $t2,$t3
            mflo $t5
            add $t5,$t5,$s2
            lw $s1,0($t5)#arr[j]
            
            blt $s0,$s1,jump#if arr[min_index]<arr[j] then jumps jump
            move $t1,$t2#min_index=i
        jump:
            #inner loop's control
            addi $t2,$t2,1
            blt $t2,$s3,inner_loop

        mult $t0,$t3
        mflo $t6
        add $a0,$t6,$s2

        move $a1,$t4

        #swap section
        lw $t8,0($a0)
        lw $t9,0($a1)
        sw $t8,0($a1)
        sw $t9,0($a0)

        #outer loop's control
        addi $t8,$s3,-1
        addi $t0,$t0,1
        blt $t0,$t8,outer_loop
        jr $ra 


#a0:array
#a1:array_size
#Gets the array and size then print the console
print_array:
	addi $sp, $sp, -20 #stack opening
	sw $ra, 16($sp)
	sw $s0, 12($sp)
	sw $s1, 8($sp)
	sw $t0, 4($sp)
	sw $t1, 0($sp)

	move $s0, $a0 #copy from $a0 to $s0 array
	move $s1, $a1 #copy from $a1 to $s1 array_size
	li $t0, 0 #counter
	
	li $v0, 4
	la $a0, sortedPrint
	syscall
print_loop: #printing loop
	beq $t0, $s1,exit_print
	
	lw $t1, 0($s0)
	li $v0, 1
	la $a0, 0($t1)
	syscall
	
	li $v0, 4
	la $a0, space
	syscall
	
	addi $t0,$t0,1
	addi $s0,$s0,4
	j print_loop
exit_print:
	li $v0, 4
	la $a0, newline
	syscall

	lw $t1, 0($sp)
	lw $t0, 4($sp)
	lw $s1, 8($sp)
	lw $s0, 12($sp)
	lw $ra, 16($sp)
	addi $sp, $sp, 20  #stack closing
	jr $ra

#exiting function
exit:
    #prints the bye
    li	$v0, 4
	la	$a0, terminate
	syscall
    
    li $v0, 10
   	syscall
