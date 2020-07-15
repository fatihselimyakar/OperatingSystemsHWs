.data
read: .space 200
double_point: .asciiz ": "
space: .asciiz " "
newline: .asciiz "\n"
palindrome_str: .asciiz "Palindrome\n"
not_polindrome_str: .asciiz "Not Palindrome\n"
terminate: .asciiz "\nGoodbye...\n"
question: .asciiz "Do you want to continue (y/n)?\n"
enter: .asciiz "\nPlease enter the last word:\n"
strings: .asciiz "abandon","ability","just","able","about","above","accept","according","account","across","act","action","activity","actually","add","address","administration","admit","adult","affect","after","again","against","age","agency","agent","ago","agree","agreement","ahead","air","all","allow","almost","alone","along","already","also","although""always","American","among","amount","analysis","and","animal","another","answer","any","anyone","anything","appear","apply","approach","area","argue","arm","around","arrive","art","article","artist","as","ask","assume","at","attack","attention","attorney","audience","author","authority","available","avoid","away","baby","back","bad","bag","ball","bank","bar","base","be","beat","beautiful","because","become","bed","before","anna","civic","kayak","level","madam","mom","noon","sagas","wow","tenet"
.text
.globl main
palindrome:
    li $t1,1
    li $t0,0    
    la $s0,strings
loop:
    add $t2,$s0,$t0
    lb $t3,($t2)
    beqz $t3,pass

    #to print number of word
    move $a0,$t1
    li $v0,1
    syscall

    #to print ':'
    la $a0,double_point
    li $v0,4
    syscall

    #to print word
    la $a0,($t2)
    li $v0,4
    syscall

    #to print ':'
    la $a0,double_point
    li $v0,4
    syscall

    la $a0,($t2)
    jal _palindrome

    move $a0,$t2
    jal string_size
    addi $t0,$t0,1
    addi $t1,$t1,1
    add $t0,$t0,$v0
    j loop
pass:
    #to print question
    la $a0,question
    li $v0,4
    syscall

    #reads yes or no
    li $v0,12
    syscall

    li $t5,110
    beq $v0,$t5,exit

    #to print enter
    la $a0,enter
    li $v0,4
    syscall

    #reads name
    la $a0,read
    li $a1,20
    li $v0,8
    syscall

    la $t2,($a0)

    #to print number of word
    move $a0,$t1
    li $v0,1
    syscall

    #to print ':'
    la $a0,double_point
    li $v0,4
    syscall

    #to print word
    la $a0,($t2)
    li $v0,4
    syscall

    #to print ':'
    la $a0,double_point
    li $v0,4
    syscall

    la $a0,($t2)
    jal _palindrome

    j exit


#a0: string
#v0: returns Is palindrome or not
_palindrome:
    addi $sp, $sp, -32 #stack opening
	sw $ra, 28($sp)
	sw $t0, 24($sp)
	sw $t1, 20($sp)
	sw $t2, 16($sp)
	sw $t3, 12($sp)
    sw $t4, 8($sp)
	sw $t5, 4($sp)
    sw $t6, 0($sp)

    jal string_size
    move $a1,$v0

    li $t0,2 #t0=2
    div $a1,$t0 #lo=length/2
    mflo $t1 #t1=lo
    addi $t2,$a1,-1

palindrome_loop:
    addi $t1, $t1, -1 # $t0 = $t1 - 1
    add $t3,$a0,$t1
    sub $t5,$t2,$t1
    add $t5,$a0,$t5
    lb $t4,($t3)
    lb $t6,($t5)
    bne $t4,$t6,is_not_palindrome
    bne	$t1, $zero, palindrome_loop # if  0!= $t1 then palindrome_loop
is_palindrome:
    li $v0,1

    #to print Palindrome
    la $a0,palindrome_str
    li $v0,4
    syscall

    lw $t6, 0($sp)
    lw $t5, 4($sp)
    lw $t4, 8($sp)
    lw $t3, 12($sp)
    lw $t2, 16($sp)
    lw $t1, 20($sp)
    lw $t0, 24($sp)
    lw $ra, 28($sp)
	addi $sp, $sp, 32  #stack closing

    jr $ra
is_not_palindrome:
    li $v0,0

    #to print Palindrome
    la $a0,not_polindrome_str
    li $v0,4
    syscall

    lw $t6, 0($sp)
    lw $t5, 4($sp)
    lw $t4, 8($sp)
    lw $t3, 12($sp)
    lw $t2, 16($sp)
    lw $t1, 20($sp)
    lw $t0, 24($sp)
    lw $ra, 28($sp)
	addi $sp, $sp, 32  #stack closing

    jr $ra

#a0: string
#v0: size
string_size:
    addi $sp, $sp, -16 #stack opening
	sw $ra, 12($sp)
	sw $t0, 8($sp)
	sw $t1, 4($sp)
	sw $t2, 0($sp)
    li $t0,0
size_loop:    
    add $t1,$a0,$t0
    lb $t2,($t1)
    li $t3,10 #newline ascii
    beq $t2,$t3,return_size
    beqz $t2,return_size
    addi $t0,$t0,1
    j size_loop
return_size:
    move $v0,$t0

    lw $t2, 0($sp)
    lw $t1, 4($sp)
    lw $t0, 8($sp)
    lw $ra, 12($sp)
	addi $sp, $sp, 16  #stack closing

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