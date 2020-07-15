.data
array: .word 10 20 22 50 51 60 75
searchingValue: .word 75
leftHalfIndex: .word 0
rightHalfIndex: .word 6


space: .asciiz " "
newline: .asciiz "\n"
searchArrayPrint: .asciiz "\nBinary searching array is:"
searchValuePrint: .asciiz "Searching value is:"
terminate: .asciiz "\nBinary Search Finished.\n"
output: .asciiz "\nOutput:"
.text

.globl main
BinarySearch:
    #Array print
    la $a0,array #array's itself
    li $a1,7 #size of array
    jal print_array

    #li $v0,5
	#syscall

	#sw $v0,searchingValue

    #String print
    la $a0,searchValuePrint
    li $v0,4
    syscall

    #Integer print
    lw $a0,searchingValue
    li $v0,1
    syscall

    #Binary search parameters
    la $a0,array
    lw $a1,searchingValue
    lw $a2,leftHalfIndex
    lw $a3,rightHalfIndex

    #Binary search
    jal binSearch
    jal exit 

#a0:searching array
#a1:searching number
#a2:left
#a3:right
binSearch:
    move $s0,$a0
    move $s1,$a1
    move $s2,$a2
    move $s3,$a3
loop:
    slt $s4,$s3,$s2 #if right<left
    bne $s4,$zero,notFound
    sub $t0,$s3,$s2 #r-l

    li $t6,2
    div $t0,$t6#(r-l)/2
    mflo $t2

    add $t1,$s2,$t2# $t1=middle

    li $t6,4
    mult $t1,$t6# middle*4 for getting middle indexed element
	mflo $t3

    add $t4,$s0,$t3 	
    lw $t5,0($t4)# array[middle]

    beq $t5,$s1,found # arr[mid] == x then return mid
    
    slt $t7,$t5,$s1
    bne $t7,$zero,left# (arr[m] < x)
right:
    addi $s3,$t1,-1
    j loop
left:
    addi $s2,$t1,1
    j loop
found:
    li $v0, 4
	la $a0, output
	syscall

    li $v0, 1
	move $a0, $t1
	syscall

    move $v0,$t1
    jr $ra
notFound:
    li	$v0, 4
	la	$a0, output
	syscall

    li $v0, 1
	li $a0, -1
	syscall

    li $v0,-1
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

    li $v0, 23
   	syscall
    
    #li $v0, 10
   	#syscall





