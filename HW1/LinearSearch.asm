.data
array: .word 60 20 22 75 50 51 10
searchingValue: .word 0
size: .word 7


space: .asciiz " "
newline: .asciiz "\n"
searchArrayPrint: .asciiz "\nSearching array is:"
searchValuePrint: .asciiz "Searching value is:"
terminate: .asciiz "\nLinear search finished.\n"
fnd: .asciiz "\nFound in index:"
nfnd: .asciiz "\nNot found!"
.text

.globl main
LinearSearch:
	#Array print
    la $a0,array #array's itself
    lw $a1,size #size of array
    jal print_array

	#String print
    la $a0,searchValuePrint
    li $v0,4
    syscall

	#Integer print
    lw $a0,searchingValue
    li $v0,1
    syscall

	#Linear search parameters
    la $a0,array
    lw $a1,searchingValue
    lw $a2,size

	#Linear search
    jal linear_search
    jal exit 

#a0:searching array
#a1:searching value
#a2:array size
#searchs the array then if find returns index of value,else returns -1
linear_search:
	addi $sp, $sp, -24 #stack opening
	sw $ra, 20($sp)
	sw $s1, 16($sp)
	sw $t0, 12($sp)
	sw $t1, 8($sp)
	sw $t2, 4($sp)
	sw $t3, 0($sp)	
	
	la $s1, 0($a0)# assigns array1's address to s1 register(LA ASSIGNS THE ADDRESS OF .WORD)
	move $t0, $a1 #searching value
	li $t1, 0 #counter
	move $t2, $a2 #arraysize (LW ASSIGNS THE CONTENT OF .WORD)
loop:	
    beq $t1, $t2,not_found #if there is not find value
	lw $t3, 0($s1) #s1 register's content assigns to t3 #burda bi sıkıntı olabilir
	beq $t0, $t3,found #if found value and searching value is same then jumps found.
	addi $t1, $t1,1 #counter adds by one
	addi $s1, $s1,4 ##go to next element of array
	j loop 
not_found:
    li $v0, 4
	la $a0, nfnd
	syscall
	li $v0, -1 #returns -1
	j exit_ls
found:	
    li $v0, 4
	la $a0, fnd
	syscall

    li $v0, 1
	move $a0, $t1
	syscall

    move $v0, $t1 #returns index of found value
	j exit_ls
exit_ls:
    lw $t3, 0($sp)
	lw $t2, 4($sp)
	lw $t1, 8($sp)
	lw $t0, 12($sp)
	lw $s1, 16($sp)
	lw $ra, 20($sp)
	addi $sp, $sp, 24 #stack closing
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
	la $a0, searchArrayPrint
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
