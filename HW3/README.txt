runs kernels with "spim read SPIM_GTU_X.s" command
before the executing, you should "sudo make install" in spim folder.

In syscall.cpp there are new system calls:

init          	       : Initialize the process table (in the assembly file) with "0" pid 			 and main process.
fork          	       : Copies the current process and then creates the new process in 			 process table (in the assembly file) with these.
execve        	       : Loads a new executable file and then changes the created process 			 infos to new loaded process.
waitpid       	       : Waits for the given process to finish.
process_exit  	       : Controls the process if there is no process in process table, 				 then system finishes.
delete_process	       : Deletes the current running process and if there are any process 			 in process table, then changes the process else system finishes.
rng           	       : Creates the new integer random number bounded by given integer 			 number.
save_the_state         : Saves the machines current state to 							 ProcessTable[running_process].
round_robin_scheduling : Finds the another not empty place in ProcessTable for change.
change_the_process     : Updates the current registers and machine informations by 			         ProcessTable[$a0] process

Note : There can be some warnings in the execution because of the program's timing. Although I minimized ,by using flags, the parts that are not optimized, it warns that the places where the interrupt will enter cannot be predicted and enters in critical places. If it warns, try running it again.