runs kernels with "spim read SPIM_GTU_X.s" command
before the executing, you should "sudo make install" in spim folder.

In syscall.cpp there are new system calls:

init          : Initialize the process table with "0" pid and main process.
fork          : Copies the current process and then creates the new process in process 			table with these.
execve        : Loads a new executable file and then changes the created process infos 			to new loaded process.
waitpid       : Waits for the given process to finish.
process_exit  : Controls the process if there is no process in process table, then system 		finishes.
delete_process: Deletes the current running process and if there are any process in 			process table, then changes the process else system finishes.
rng           :	Creates the new integer random number bounded by given integer number.

Note : There can be some warnings in the execution because of the program's timing. Although I minimized the parts that are not optimized, it warns that the places where the interrupt will enter cannot be predicted and enters in critical places. If it warns, try running it again.