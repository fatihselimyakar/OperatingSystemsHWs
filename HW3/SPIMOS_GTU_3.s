.data

terminate: .asciiz "\nGoodbye...\n"
c_switch: .asciiz "Context Switch\n"

running_process: .word 4
running_process_count: .word 4
all_process_count: .word 4
handler_flag: .word 4
waitpid_flag: .word 4
waitpid_process: .word 4
waitpid_end_addres: .word 4
process_table_size: .word 4
critical_region_flag: .word 4

#Process table includes 10 process control block
pcb0: .align 2 
      .space 200
pcb1: .align 2 
      .space 200
pcb2: .align 2 
      .space 200
pcb3: .align 2 
      .space 200
pcb4: .align 2 
      .space 200
pcb5: .align 2 
      .space 200
pcb6: .align 2 
      .space 200
pcb7: .align 2 
      .space 200
pcb8: .align 2 
      .space 200
pcb9: .align 2 
      .space 200

linearSearch: .asciiz "LinearSearch.asm"
binarySearch: .asciiz "BinarySearch.asm"
collatz: .asciiz "Collatz.asm"
palindrome: .asciiz "Palindrome.asm"
rng: .asciiz "RNG:"

.text
.globl main
#Context switch function
context_switch:
    # opens the critical region like mutex
    li $t0,1
    sw $t0,critical_region_flag

    # prints context switch text
    la $a0,c_switch
    li $v0,4
    syscall

    # calls Save State syscall
    li $v0,25
    syscall

    # calls Round Robin Scheduling syscall
    li $v0,26
    syscall

    # $v0 is the new process id
    # calls the Change Process syscall
    move $a0,$v0
    li $v0,27
    syscall

    # closes the critical region 
    sw $zero,critical_region_flag

    # jumps the new PC
    jr $s7


.globl main
main:
    #initialize the global variables(all of other variables are 0)
    li $t0,10
    sw $t0,process_table_size

    li $t0,1
    sw $t0,handler_flag

    li $t0,-1
    sw $t0,waitpid_process

    #RNG printing
    li $v0,4
    la $a0,rng
    syscall

    #First rng number=$s0
    li $v0,24
    li $a0,4
    syscall
    move $s0,$v0
    
    #Second number ($s1=($s0+1)%4)
    addi $s1,$s0,1
    div  $s1,$a0 # for modulo dividing
    mfhi $s1 # gets the modulo to $s1 

    #Third number ($s2=($s0+2)%4)
    addi $s2,$s0,2
    div  $s2,$a0 # for modulo dividing
    mfhi $s2 # gets the modulo to $s2

    #init syscall
    li $v0,22
    syscall

    #waitpid
    li $v0,20
    li $a0,0
    syscall

    # Assignments for branch comparisons
    li $t1,1
    li $t2,2
    li $t0,3


    #Basic if-else structure for running 3 times the chosen programs
    bne $s0,$zero,process1_not_linear 
    la $a0,linearSearch #if the first process is linear search then this part runs,else jumps under conditions
    jal execute3times
process1_not_linear:   
    bne $s0,$t1,process1_not_binary
    la $a0,binarySearch #if the first process is binary search then this part runs,else jumps under conditions
    jal execute3times
process1_not_binary:
    bne $s0,$t2,process1_not_collatz
    la $a0,collatz #if the first process is collatz then this part runs,else jumps under conditions
    jal execute3times
process1_not_collatz:
    bne $s0,$t0,process1_not_palindrome
    la $a0,palindrome #if the first process is palindrome then this part runs,else jumps under conditions
    jal execute3times
process1_not_palindrome:
    bne $s1,$zero,process2_not_linear 
    la $a0,linearSearch #if the second process is linear search then this part runs,else jumps under conditions
    jal execute3times
process2_not_linear:
    bne $s1,$t1,process2_not_binary
    la $a0,binarySearch #if the second process is binary search then this part runs,else jumps under conditions
    jal execute3times
process2_not_binary:
    bne $s1,$t2,process2_not_collatz
    la $a0,collatz #if the second process is collatz then this part runs,else jumps under conditions
    jal execute3times
process2_not_collatz:
    bne $s1,$t0,process2_not_palindrome 
    la $a0,palindrome #if the second process is palindrome then this part runs,else jumps under conditions
    jal execute3times
process2_not_palindrome:
    bne $s2,$zero,process3_not_linear 
    la $a0,linearSearch #if the third process is linear search then this part runs,else jumps under conditions
    jal execute3times
process3_not_linear:
    bne $s2,$t1,process3_not_binary
    la $a0,binarySearch #if the third process is binary search then this part runs,else jumps under conditions
    jal execute3times
process3_not_binary:
    bne $s2,$t2,process3_not_collatz
    la $a0,collatz #if the third process is collatz then this part runs,else jumps under conditions
    jal execute3times
process3_not_collatz:
    bne $s2,$t0,process3_not_palindrome 
    la $a0,palindrome #if the third process is palindrome then this part runs,else jumps under conditions
    jal execute3times
process3_not_palindrome:
    jal exitSpim

#Runs the 3 times choosen program
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

# Sets the $a1 th struct's $a2 offset variable with $a0 value.
# a0: the value
# a1: which pcb
# a2: offset
set_struct:
    addi $sp, $sp, -20 #stack opening
	sw $ra, 16($sp)
	sw $t0, 12($sp)
	sw $t1, 8($sp)
    sw $t2, 4($sp)
	sw $t3, 0($sp)

    la $t0,pcb0
    li $t1,200
    mult $a1, $t1 #  $a1 * $t1 = Hi and Lo registers
    mflo $t2 # copy Lo to $t2
    add $t3,$t0,$t2 # it finds pcb0,1,2,3,4
    add $t3,$t3,$a2
    sw  $a0, ($t3) # pcbX.struct_variable=$a0

    lw $t3, 0($sp)
    lw $t2, 4($sp)
    lw $t1, 8($sp)
    lw $t0, 12($sp)
    lw $ra, 16($sp)
	addi $sp, $sp, 20  #stack closing
    jr $ra

# Gets the $a0 th struct's $a1 offset variable, returns $v0.
#a0: which pcb
#a1: offset
#v0: return 
get_struct:
    addi $sp, $sp, -20 #stack opening
	sw $ra, 16($sp)
	sw $t0, 12($sp)
	sw $t1, 8($sp)
    sw $t2, 4($sp)
	sw $t3, 0($sp)

    la $t0,pcb0
    li $t1,200
    mult $a0, $t1 #  $a0 * $t1 = Hi and Lo registers
    mflo $t2 # copy Lo to $t2
    add $t3,$t0,$t2 # it finds pcb0,1,2,3,4
    add $t3,$t3,$a2
    lw  $v0, ($t3) # $v0=pcbX.struct_variable

    lw $t3, 0($sp)
    lw $t2, 4($sp)
    lw $t1, 8($sp)
    lw $t0, 12($sp)
    lw $ra, 16($sp)
	addi $sp, $sp, 20  #stack closing
    jr $ra

#a0: the id
#a1: which pcb
set_process_id:
    addi $sp, $sp, -4 #stack opening
	sw $ra, 0($sp)

    li $a2,0
    jal set_struct

	lw $ra, 0($sp)
    addi $sp, $sp, 4 #stack opening
    jr $ra

#a0: which pcb
#v0: return id
get_process_id:
    addi $sp, $sp, -4 #stack opening
    sw $ra, 0($sp)

    li $a2,0
    jal get_struct
    
    lw $ra, 0($sp)
    addi $sp, $sp, 4 #stack opening
    jr $ra

#a0: the name
#a1: which pcb
set_process_name:
    addi $sp, $sp, -4 #stack opening
	sw $ra, 0($sp)

    li $a2,4
    jal set_struct

	lw $ra, 0($sp)
    addi $sp, $sp, 4 #stack opening
    jr $ra
    
#a0: which pcb
#v0: return name
get_process_name:
    addi $sp, $sp, -4 #stack opening
    sw $ra, 0($sp)

    li $a2,4
    jal get_struct
    
    lw $ra, 0($sp)
    addi $sp, $sp, 4 #stack opening
    jr $ra

#a0: the PC
#a1: which pcb
set_program_counter:
    addi $sp, $sp, -4 #stack opening
	sw $ra, 0($sp)

    li $a2,28
    jal set_struct

	lw $ra, 0($sp)
    addi $sp, $sp, 4 #stack opening
    jr $ra
    
#a0: which pcb
#v0: return PC
get_program_counter:
    addi $sp, $sp, -4 #stack opening
	sw $ra, 0($sp)

    li $a2,28
    jal get_struct

    lw $ra, 0($sp)
    addi $sp, $sp, 4 #stack opening
    jr $ra

#a0: the text PC
#a1: which pcb
set_program_text_pc:
    addi $sp, $sp, -4 #stack opening
	sw $ra, 0($sp)

    li $a2,32
    jal set_struct

	lw $ra, 0($sp)
    addi $sp, $sp, 4 #stack opening
    jr $ra
    
#a0: which pcb
#v0: return text PC
get_program_text_pc:
    addi $sp, $sp, -4 #stack opening
	sw $ra, 0($sp)

    li $a2,32
    jal get_struct

    lw $ra, 0($sp)
    addi $sp, $sp, 4 #stack opening
    jr $ra

#a0: the ent text PC
#a1: which pcb
set_program_end_text_pc:
    addi $sp, $sp, -4 #stack opening
	sw $ra, 0($sp)

    li $a2,36
    jal set_struct

	lw $ra, 0($sp)
    addi $sp, $sp, 4 #stack opening
    jr $ra
    
#a0: which pcb
#v0: return end text PC
get_program_end_text_pc:
    addi $sp, $sp, -4 #stack opening
	sw $ra, 0($sp)

    li $a2,36
    jal get_struct

    lw $ra, 0($sp)
    addi $sp, $sp, 4 #stack opening
    jr $ra

#a0: the data PC
#a1: which pcb
set_program_data_pc:
    addi $sp, $sp, -4 #stack opening
	sw $ra, 0($sp)

    li $a2,40
    jal set_struct

	lw $ra, 0($sp)
    addi $sp, $sp, 4 #stack opening
    jr $ra
    
#a0: which pcb
#v0: return data PC
get_program_data_pc:
    addi $sp, $sp, -4 #stack opening
	sw $ra, 0($sp)

    li $a2,40
    jal get_struct

    lw $ra, 0($sp)
    addi $sp, $sp, 4 #stack opening
    jr $ra

#a0: the stack pointer addres
#a1: which pcb
set_stack_pointer_addr:
    addi $sp, $sp, -4 #stack opening
	sw $ra, 0($sp)

    li $a2,44
    jal set_struct

	lw $ra, 0($sp)
    addi $sp, $sp, 4 #stack opening
    jr $ra
    
#a0: which pcb
#v0: return stack pointer addres
get_stack_pointer_addr:
    addi $sp, $sp, -4 #stack opening
	sw $ra, 0($sp)

    li $a2,44
    jal get_struct

    lw $ra, 0($sp)
    addi $sp, $sp, 4 #stack opening
    jr $ra

#a0: the process state
#a1: which pcb
set_process_state:
    addi $sp, $sp, -4 #stack opening
	sw $ra, 0($sp)

    li $a2,48
    jal set_struct

	lw $ra, 0($sp)
    addi $sp, $sp, 4 #stack opening
    jr $ra
    
#a0: which pcb
#v0: return process state
get_process_state:
    addi $sp, $sp, -4 #stack opening
	sw $ra, 0($sp)

    li $a2,48
    jal get_struct

    lw $ra, 0($sp)
    addi $sp, $sp, 4 #stack opening
    jr $ra

#a0: the parent process
#a1: which pcb
set_parent_process:
    addi $sp, $sp, -4 #stack opening
	sw $ra, 0($sp)

    li $a2,60
    jal set_struct

	lw $ra, 0($sp)
    addi $sp, $sp, 4 #stack opening
    jr $ra
    
#a0: which pcb
#v0: return parent process
get_parent_process:
    addi $sp, $sp, -4 #stack opening
	sw $ra, 0($sp)

    li $a2,60
    jal get_struct

    lw $ra, 0($sp)
    addi $sp, $sp, 4 #stack opening
    jr $ra

#a0: the r array's start addres
#a1: which pcb
set_r_temp:
    addi $sp, $sp, -4 #stack opening
	sw $ra, 0($sp)

    li $a2,64
    jal set_struct

	lw $ra, 0($sp)
    addi $sp, $sp, 4 #stack opening
    jr $ra
    
#a0: which pcb
#v0: return start value of r array
get_r_temp:
    addi $sp, $sp, -4 #stack opening
	sw $ra, 0($sp)

    li $a2,64
    jal get_struct

    lw $ra, 0($sp)
    addi $sp, $sp, 4 #stack opening
    jr $ra

#a0: the hi value
#a1: which pcb
set_hi:
    addi $sp, $sp, -4 #stack opening
	sw $ra, 0($sp)

    li $a2,192
    jal set_struct

	lw $ra, 0($sp)
    addi $sp, $sp, 4 #stack opening
    jr $ra
    
#a0: which pcb
#v0: return hi value
get_hi:
    addi $sp, $sp, -4 #stack opening
	sw $ra, 0($sp)

    li $a2,192
    jal get_struct

    lw $ra, 0($sp)
    addi $sp, $sp, 4 #stack opening
    jr $ra

#a0: the lo value
#a1: which pcb
set_lo:
    addi $sp, $sp, -4 #stack opening
	sw $ra, 0($sp)

    li $a2,196
    jal set_struct

	lw $ra, 0($sp)
    addi $sp, $sp, 4 #stack opening
    jr $ra
    
#a0: which pcb
#v0: return lo value
get_lo:
    addi $sp, $sp, -4 #stack opening
	sw $ra, 0($sp)

    li $a2,196
    jal get_struct

    lw $ra, 0($sp)
    addi $sp, $sp, 4 #stack opening
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

    #process exit syscall
    #loop:
    li $v0,21
    syscall
    #j loop

