/* SPIM S20 MIPS simulator.
   Execute SPIM syscalls, both in simulator and bare mode.
   Execute MIPS syscalls in bare mode, when running on MIPS systems.
   Copyright (c) 1990-2010, James R. Larus.
   All rights reserved.

   Redistribution and use in source and binary forms, with or without modification,
   are permitted provided that the following conditions are met:

   Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

   Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation and/or
   other materials provided with the distribution.

   Neither the name of the James R. Larus nor the names of its contributors may be
   used to endorse or promote products derived from this software without specific
   prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
   GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
   OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#ifndef _WIN32
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>

#ifdef _WIN32
#include <io.h>
#endif

#include "spim.h"
#include "string-stream.h"
#include "inst.h"
#include "reg.h"
#include "mem.h"
#include "sym-tbl.h"
#include "syscall.h"
#include "data.h"
#include "spim-utils.h"

#include <iostream>
using namespace std;

#ifdef _WIN32
/* Windows has an handler that is invoked when an invalid argument is passed to a system
   call. https://msdn.microsoft.com/en-us/library/a9yf33zb(v=vs.110).aspx

   All good, except that the handler tries to invoke Watson and then kill spim with an exception.

   Override the handler to just report an error.
*/

#include <stdio.h>
#include <stdlib.h>
#include <crtdbg.h>

void myInvalidParameterHandler(const wchar_t* expression,
   const wchar_t* function,
   const wchar_t* file,
   unsigned int line,
   uintptr_t pReserved)
{
  if (function != NULL)
    {
      run_error ("Bad parameter to system call: %s\n", function);
    }
  else
    {
      run_error ("Bad parameter to system call\n");
    }
}

static _invalid_parameter_handler oldHandler;

void windowsParameterHandlingControl(int flag )
{
  static _invalid_parameter_handler oldHandler;
  static _invalid_parameter_handler newHandler = myInvalidParameterHandler;

  if (flag == 0)
    {
      oldHandler = _set_invalid_parameter_handler(newHandler);
      _CrtSetReportMode(_CRT_ASSERT, 0); // Disable the message box for assertions.
    }
  else
    {
      newHandler = _set_invalid_parameter_handler(oldHandler);
      _CrtSetReportMode(_CRT_ASSERT, 1);  // Enable the message box for assertions.
    }
}
#endif

/* Process table and Global variable's data memory addresses */
#define CONTEXT_SWITCH_PRINT 268501005
#define RUNNING_PROCESS_ADDRES 268501024
#define RUNNING_PROCESS_COUNT_ADDRES 268501028
#define ALL_PROCESS_COUNT_ADDRES 268501032
#define HANDLER_FLAG 268501036
#define WAITPID_FLAG 268501040
#define WAITPID_PROCESS 268501044
#define WAITPID_END_ADDRES 268501048
#define PROCESS_TABLE_SIZE_ADDRES 268501052
#define CRITICAL_REGION_FLAG 268501056
#define PCB0_ADDRES 268501060

/* Context Switch Function text addres */
#define CONTEXT_SWITCH_ADDRES 0x00400024

/* Size of a Process Control Block */
#define SIZEOF_PCB 200

/* Addres finding macros */
#define PCB_ADDRESS(X) (PCB0_ADDRES+X*SIZEOF_PCB) /* Finds ProcessTable[X] addres */
#define PROCESS_ID(ADDRES) (ADDRES)               /* Finds given Process Control Block's Process Id addres */
#define PROCESS_NAME(ADDRES) (ADDRES+4)           /* Finds given Process Control Block's Process Name addres */
#define PC(ADDRES) (ADDRES+28)                    /* Finds given Process Control Block's PC addres */
#define TEXT_PC(ADDRES) (ADDRES+32)               /* Finds given Process Control Block's Text PC addres */
#define END_TEXT_PC(ADDRES) (ADDRES+36)           /* Finds given Process Control Block's End Text PC addres */
#define DATA_PC(ADDRES) (ADDRES+40)               /* Finds given Process Control Block's Data PC addres */
#define STACK_ADDRES(ADDRES) (ADDRES+44)          /* Finds given Process Control Block's Stack addres */
#define PROCESS_STATE(ADDRES) (ADDRES+48)         /* Finds given Process Control Block's Process State addres */
#define PARENT_PROCESS(ADDRES) (ADDRES+60)        /* Finds given Process Control Block's Data Parent Process addres */
#define REGISTERS(ADDRES) (ADDRES+64)             /* Finds given Process Control Block's Registers start addres */
#define HI(ADDRES) (ADDRES+192)                   /* Finds given Process Control Block's HI register addres */
#define LO(ADDRES) (ADDRES+196)                   /* Finds given Process Control Block's LO register addres */

/* Writes string in the given data memory address usind set_mem_byte() */
void write_string_addres(char string[],int start_address){
  int i;
  for(i=0;i<(int)strlen(string)+1;++i){
    /* Writes '\0' last index */
    if(i==(int)strlen(string)){
      set_mem_byte(start_address+i,'\0');
    }
    /* Writes addresses the string[i] character */
    else{
      set_mem_byte(start_address+i,string[i]);
    }
  }
}

/* Prints out the given pcb_num's process infos */
void print_pcb_infos(int pcb_num){
  int addres=PCB0_ADDRES+pcb_num*SIZEOF_PCB; /* Finds wanted pcb's addres */
  printf("\nProcess id     : %d\n",read_mem_word(PROCESS_ID(addres)));
  printf("Process name   : %s\n",(char*)mem_reference(PROCESS_NAME(addres)));
  printf("PC             : %x\n",read_mem_word(PC(addres)));
  printf("text_pc        : %x\n",read_mem_word(TEXT_PC(addres)));
  printf("end_text_pc    : %x\n",read_mem_word(END_TEXT_PC(addres)));
  printf("data_pc        : %x\n",read_mem_word(DATA_PC(addres)));
  printf("Stack_addres   : %x\n",read_mem_word(STACK_ADDRES(addres)));
  printf("process_state  : %s\n",(char*)mem_reference(PROCESS_STATE(addres)));
  printf("parent_process : %d\n\n",read_mem_word(PARENT_PROCESS(addres)));
}

/* Saves the current registers to process table in the memory */
void register_to_memory(int pcb_num){
  int addres=PCB0_ADDRES+pcb_num*SIZEOF_PCB; /* Finds wanted pcb's addres */
  int i=0;                                   /* index of R[] */
  int offset=0;                              /* Addres offset */
  for(;i<32;++i,offset+=4){                  /* Sets the process table addres to current registers */
    set_mem_word(REGISTERS(addres)+offset,R[i]);
  }
}

/* Saves the process table in the memory to current registers */
void memory_to_register(int pcb_num){
  int addres=PCB0_ADDRES+pcb_num*SIZEOF_PCB; /* Finds wanted pcb's addres */
  int i=0;                                   /* index of R[] */
  int offset=0;                              /* Addres offset */
  for(;i<32;++i,offset+=4){                  /* Sets the registers to process table addres */
    R[i]=read_mem_word(REGISTERS(addres)+offset);
  }
}

/* Controls is finished */
bool is_finish(){
  return read_mem_word(RUNNING_PROCESS_COUNT_ADDRES) == 0 ? true : false;
}

/* Deletes the current running process */
void delete_process(){
  set_mem_word(PC(PCB_ADDRESS(read_mem_word(RUNNING_PROCESS_ADDRES))),0);                   /* I assume if PC=0,the process table entry is empty. Because of that I assing the PC to 0 in this case */
  set_mem_word(RUNNING_PROCESS_COUNT_ADDRES,read_mem_word(RUNNING_PROCESS_COUNT_ADDRES)-1); /* Decrease the value of running process count int the assembly file (kernel) */
  printf("Process %d deleted. After the delete process count: %d\n",read_mem_word(RUNNING_PROCESS_ADDRES),read_mem_word(RUNNING_PROCESS_COUNT_ADDRES));
}

/* Changes the current process to new process */
void exchange_func(){
  if(read_mem_word(HANDLER_FLAG)){
    int running_process=PCB_ADDRESS(read_mem_word(RUNNING_PROCESS_ADDRES));                 /* Finds the running process's addres in the assembly file (kernel). */
    printf("%s",(char*)mem_reference(CONTEXT_SWITCH_PRINT));
    write_string_addres("READY",PROCESS_STATE(running_process));                            /* Changes the process state */
    printf("Before the process change:\n");
    print_pcb_infos(read_mem_word(RUNNING_PROCESS_ADDRES));                                 /* Prints the current process's informations */
    if(read_mem_word(PC(running_process))!=0 && (unsigned int)read_mem_word(END_TEXT_PC(running_process))<=PC ){
      delete_process();                                                                     /* If there is overflow, then deletes the process */
    }
    if(read_mem_word(PC(running_process))!=0){                                              /* If current process is not empty then saves the current data to process table */ 
      set_mem_word(PC(running_process),PC);                                                 /* Saves all information into ProcessTable[running process] */
      set_mem_word(STACK_ADDRES(running_process),R[29]);
      write_string_addres("READY",PROCESS_STATE(running_process));
      register_to_memory(read_mem_word(RUNNING_PROCESS_ADDRES));//degistim
      set_mem_word(HI(running_process),HI);
      set_mem_word(LO(running_process),LO);
      set_mem_word(DATA_PC(running_process),current_data_pc());
      set_mem_word(END_TEXT_PC(running_process),current_text_pc());
    }

    /* Round Robin Scheduling */
    /* Finds the another not empty place in ProcessTable for change */
    int i;
    for(i=(read_mem_word(RUNNING_PROCESS_ADDRES)+1)%read_mem_word(PROCESS_TABLE_SIZE_ADDRES);i<read_mem_word(PROCESS_TABLE_SIZE_ADDRES);++i){
      if(read_mem_word(PC(PCB_ADDRESS(i)))!=0){                                             /* If not empty then breaks. */
        break;
      }
      if(i==read_mem_word(PROCESS_TABLE_SIZE_ADDRES)-1){                                    /* Restart to begin */
        i=-1;
      }
    }

    /* Waitpid syscall flag's handling */
    if(PC>=(unsigned int)read_mem_word(WAITPID_END_ADDRES)){  
      set_mem_word(WAITPID_FLAG,0);
    }
    if(read_mem_word(WAITPID_FLAG)==1){
      i=read_mem_word(WAITPID_PROCESS);
    }

    /* Updates the current registers and machine informations by ProcessTable[i] process */
    char state[]="RUNNING";
    int new_address=PCB_ADDRESS(i);
    PC=read_mem_word(PC(new_address));
    R[29]=read_mem_word(STACK_ADDRES(new_address));
    memory_to_register(i);
    HI=read_mem_word(HI(new_address));
    LO=read_mem_word(LO(new_address));
    set_text_pc(read_mem_word(END_TEXT_PC(new_address)));
    set_data_pc(read_mem_word(DATA_PC(new_address)));
    write_string_addres(state,PROCESS_STATE(new_address));

    /* Updates the current running process to i */
    set_mem_word(RUNNING_PROCESS_ADDRES,i);

    printf("After the process change:\n");
    print_pcb_infos(read_mem_word(RUNNING_PROCESS_ADDRES));

  }
  set_mem_word(HANDLER_FLAG,1);
}

/* Timer handler */
void SPIM_timerHandler()
{
  printf("\nSpim Timer Handler\n");
  //R[23]=PC;
  //PC=CONTEXT_SWITCH_ADDRES;
  exchange_func();
}
/* Decides which syscall to execute or simulate.  Returns zero upon
   exit syscall and non-zero to continue execution. */
int
do_syscall ()
{
#ifdef _WIN32
    windowsParameterHandlingControl(0);
#endif
  /* If there is any overflow in current process then deletes the process */
  int running_process=PCB_ADDRESS(read_mem_word(RUNNING_PROCESS_ADDRES));
  if(read_mem_word(PC(running_process))!=0 && (unsigned int)read_mem_word(END_TEXT_PC(running_process))<=PC ){
    R[REG_V0]=PROCESS_EXIT_SYSCALL;
  }

  /* Syscalls for the source-language version of SPIM.  These are easier to
     use than the real syscall and are portable to non-MIPS operating
     systems. */
    
  /* I change the process states in all of under the I/O syscalls to BLOCKED and RUNNING */

  switch (R[REG_V0])
    {
    case PRINT_INT_SYSCALL:
      if(read_mem_word(PC(running_process))!=0)
        write_string_addres("BLOCKED",PROCESS_STATE(running_process));
      if(R[REG_A0]>=0x10000000){
        write_output (console_out, "%s",(char*)mem_reference (R[REG_A0]));
      }
      else{
        write_output (console_out, "%d", R[REG_A0]);
      }
      if(read_mem_word(PC(running_process))!=0)
        write_string_addres("RUNNING",PROCESS_STATE(running_process));
      break;

    case PRINT_FLOAT_SYSCALL:
      {
	float val = FPR_S (REG_FA0);
  if(read_mem_word(PC(running_process))!=0)
    write_string_addres("BLOCKED",PROCESS_STATE(running_process));
	write_output (console_out, "%.8f", val);
  if(read_mem_word(PC(running_process))!=0)
    write_string_addres("RUNNING",PROCESS_STATE(running_process));
	break;
      }

    case PRINT_DOUBLE_SYSCALL:
      if(read_mem_word(PC(running_process))!=0)
        write_string_addres("BLOCKED",PROCESS_STATE(running_process));
      write_output (console_out, "%.18g", FPR[REG_FA0 / 2]);
      if(read_mem_word(PC(running_process))!=0)
        write_string_addres("RUNNING",PROCESS_STATE(running_process));
      break;

    case PRINT_STRING_SYSCALL:
      if(read_mem_word(PC(running_process))!=0)
        write_string_addres("BLOCKED",PROCESS_STATE(running_process));
      if(0x10000000<=R[REG_A0]){
        write_output (console_out, "%s", (char*)mem_reference (R[REG_A0]));
      }
      else{
        write_output (console_out, "%d ", R[REG_A0]);
      }
      if(read_mem_word(PC(running_process))!=0)
        write_string_addres("RUNNING",PROCESS_STATE(running_process));
      break;

    case READ_INT_SYSCALL:
      {
	static char str [256];
  if(read_mem_word(PC(running_process))!=0)
    write_string_addres("BLOCKED",PROCESS_STATE(running_process));
	read_input (str, 256);
	R[REG_RES] = atol (str);
  if(read_mem_word(PC(running_process))!=0)
    write_string_addres("RUNNING",PROCESS_STATE(running_process));
	break;
      }

    case READ_FLOAT_SYSCALL:
      {
	static char str [256];
  if(read_mem_word(PC(running_process))!=0)
    write_string_addres("BLOCKED",PROCESS_STATE(running_process));
	read_input (str, 256);
	FPR_S (REG_FRES) = (float) atof (str);
  if(read_mem_word(PC(running_process))!=0)
    write_string_addres("RUNNING",PROCESS_STATE(running_process));
	break;
      }

    case READ_DOUBLE_SYSCALL:
      {
	static char str [256];
  if(read_mem_word(PC(running_process))!=0)
    write_string_addres("BLOCKED",PROCESS_STATE(running_process));
	read_input (str, 256);
	FPR [REG_FRES] = atof (str);
  if(read_mem_word(PC(running_process))!=0)
    write_string_addres("RUNNING",PROCESS_STATE(running_process));
	break;
      }

    case READ_STRING_SYSCALL:
      {
  if(read_mem_word(PC(running_process))!=0)
    write_string_addres("BLOCKED",PROCESS_STATE(running_process));
	read_input ( (char *) mem_reference (R[REG_A0]), R[REG_A1]);
	data_modified = true;
  char *string=(char*)mem_reference(R[REG_A0]);
  if(string[strlen(string)-1]=='\n'){
    string[strlen(string)-1]='\0';
  }
  if(read_mem_word(PC(running_process))!=0)
    write_string_addres("RUNNING",PROCESS_STATE(running_process));
	break;
      }

    case SBRK_SYSCALL:
      {
	mem_addr x = data_top;
	expand_data (R[REG_A0]);
	R[REG_RES] = x;
	data_modified = true;
	break;
      }

    case PRINT_CHARACTER_SYSCALL:
      if(read_mem_word(PC(running_process))!=0)
        write_string_addres("BLOCKED",PROCESS_STATE(running_process));
      write_output (console_out, "%c", R[REG_A0]);
      if(read_mem_word(PC(running_process))!=0)
        write_string_addres("RUNNING",PROCESS_STATE(running_process));
      break;

    case READ_CHARACTER_SYSCALL:
      {
	static char str [2];
  if(read_mem_word(PC(running_process))!=0)
    write_string_addres("BLOCKED",PROCESS_STATE(running_process));
  str[0]=getchar();
  R[REG_RES] = (long) str[0];
  if(read_mem_word(PC(running_process))!=0)
    write_string_addres("RUNNING",PROCESS_STATE(running_process));
	break;
      }

    case EXIT_SYSCALL:
      spim_return_value = 0;
      return (0);

    case EXIT2_SYSCALL:
      spim_return_value = R[REG_A0];	/* value passed to spim's exit() call */
      return (0);

    case OPEN_SYSCALL:
      {
#ifdef _WIN32
        R[REG_RES] = _open((char*)mem_reference (R[REG_A0]), R[REG_A1], R[REG_A2]);
#else
	R[REG_RES] = open((char*)mem_reference (R[REG_A0]), R[REG_A1], R[REG_A2]);
#endif
	break;
      }

    case READ_SYSCALL:
      {
	/* Test if address is valid */
	(void)mem_reference (R[REG_A1] + R[REG_A2] - 1);
#ifdef _WIN32
	R[REG_RES] = _read(R[REG_A0], mem_reference (R[REG_A1]), R[REG_A2]);
#else
	R[REG_RES] = read(R[REG_A0], mem_reference (R[REG_A1]), R[REG_A2]);
#endif
	data_modified = true;
	break;
      }

    case WRITE_SYSCALL:
      {
	/* Test if address is valid */
	(void)mem_reference (R[REG_A1] + R[REG_A2] - 1);
#ifdef _WIN32
	R[REG_RES] = _write(R[REG_A0], mem_reference (R[REG_A1]), R[REG_A2]);
#else
	R[REG_RES] = write(R[REG_A0], mem_reference (R[REG_A1]), R[REG_A2]);
#endif
	break;
      }

    case CLOSE_SYSCALL:
      {
#ifdef _WIN32
	R[REG_RES] = _close(R[REG_A0]);
#else
	R[REG_RES] = close(R[REG_A0]);
#endif
	break;
      }
    /* Initialize the new process in process table's zeroth process control block  */
    case INIT_SYSCALL:
      {
#ifndef _WIN32
  printf("Init Syscall Occured\n");

  /* Saves the all of machine informations to ProcessTable[0] */

  /* Process Table Savings */
  set_mem_word(PROCESS_ID(PCB0_ADDRES),0);
  write_string_addres("init",PROCESS_NAME(PCB0_ADDRES));
  set_mem_word(PC(PCB0_ADDRES),PC+4);
  write_string_addres("RUNNING",PROCESS_STATE(PCB0_ADDRES));
  set_mem_word(PARENT_PROCESS(PCB0_ADDRES),-1);
  set_mem_word(END_TEXT_PC(PCB0_ADDRES),current_text_pc());
  set_mem_word(TEXT_PC(PCB0_ADDRES),0x00400020);
  set_mem_word(DATA_PC(PCB0_ADDRES),current_data_pc());
  set_mem_word(STACK_ADDRES(PCB0_ADDRES),R[29]);
  set_mem_word(HI(PCB0_ADDRES),HI);
  set_mem_word(LO(PCB0_ADDRES),LO);
  register_to_memory(0);

  /* Updating the global variables in the assembly language */
  set_mem_word(ALL_PROCESS_COUNT_ADDRES,1);
  set_mem_word(RUNNING_PROCESS_COUNT_ADDRES,1);
  set_mem_word(RUNNING_PROCESS_ADDRES,0);

  /* Returns the process id */
  R[REG_RES]=0;
#endif
	break;
      }
    /* Allocates the new allocation in process table for the new process then fills it by current process'es copy. */
    case FORK_SYSCALL:
      {
#ifndef _WIN32
  printf("Fork Sycall Occured\n");
  int running_process_addres=PCB_ADDRESS(read_mem_word(RUNNING_PROCESS_ADDRES)); /* Finds the running process's addres in the assembly data memory */
  int free_process_addres=PCB0_ADDRES;                                           /* Free process table entry addres */
  int pcb=0;                                                                     /* Free process table entry index */
  while(read_mem_word(PC(free_process_addres))!=0){                              /* Finds the new empty process control block in the process table */
    free_process_addres+=200;
    ++pcb;
  }

  /* Filling the current process to new process table's index */
  char temp[20];
  strcpy(temp,(char*)mem_reference(PROCESS_NAME(running_process_addres)));
  write_string_addres(strcat(temp," child"),PROCESS_NAME(free_process_addres));
  set_mem_word(PROCESS_ID(free_process_addres),read_mem_word(ALL_PROCESS_COUNT_ADDRES));
  set_mem_word(PC(free_process_addres),PC);
  set_mem_word(TEXT_PC(free_process_addres),read_mem_word(TEXT_PC(running_process_addres)));
  set_mem_word(END_TEXT_PC(free_process_addres),read_mem_word(END_TEXT_PC(running_process_addres)));
  set_mem_word(DATA_PC(free_process_addres),read_mem_word(DATA_PC(running_process_addres)));
  set_mem_word(STACK_ADDRES(free_process_addres),R[29]);
  write_string_addres("READY",PROCESS_STATE(free_process_addres));
  register_to_memory(pcb);
  set_mem_word(HI(free_process_addres),HI);
  set_mem_word(LO(free_process_addres),LO);

  /* Updating the global variables in the assemblt language */
  set_mem_word(ALL_PROCESS_COUNT_ADDRES,read_mem_word(ALL_PROCESS_COUNT_ADDRES)+1);
  set_mem_word(RUNNING_PROCESS_COUNT_ADDRES,read_mem_word(RUNNING_PROCESS_COUNT_ADDRES)+1);

  /* Returns the process id */
  R[REG_RES]=pcb;
#endif
	break;
      }
    /* Loads the file in the memory and changes the current process'es informations to loaded process */
    case EXECVE_SYSCALL:
      {
#ifndef _WIN32
  printf("Execve Syscall Occured\n");
  int running_process_addres=PCB_ADDRESS((read_mem_word(RUNNING_PROCESS_COUNT_ADDRES)-1));
  /* If the running process is not init process */
  if(read_mem_word(PROCESS_ID(running_process_addres))!=0 && strcmp((char*)mem_reference(PROCESS_NAME(running_process_addres)),"init")!=0){
    char* file_name=(char *)mem_reference (R[REG_A0]);
    mem_addr start_text_addr=current_text_pc();
    /* Reads the new assembly file */
    read_assembly_file(file_name);

    /* Sets to running process's process control block to new program's informations */
    register_to_memory((read_mem_word(RUNNING_PROCESS_COUNT_ADDRES)-1));
    set_mem_word(PC(running_process_addres),start_text_addr);
    set_mem_word(TEXT_PC(running_process_addres),start_text_addr);
    set_mem_word(END_TEXT_PC(running_process_addres),current_text_pc());
    set_mem_word(DATA_PC(running_process_addres),current_data_pc());
    set_mem_word(HI(running_process_addres),HI);
    set_mem_word(LO(running_process_addres),LO);
    set_mem_word(STACK_ADDRES(running_process_addres),R[29]);
    write_string_addres(file_name,PROCESS_NAME(running_process_addres));
  }

  /* Returns the process id */
  R[REG_RES]=read_mem_word(RUNNING_PROCESS_COUNT_ADDRES)-1;
#endif
	break;
      }
    /* Controls the process if there is no process in process table, then system finishes. */
    case PROCESS_EXIT_SYSCALL:
      {
#ifndef _WIN32
      if(is_finish()){
        printf("Program Has Been Finished\n");
        force_break=true;
        spim_return_value = 0;
        return (0);
      }

#endif
  break;
      }
    /* Deletes the current running process and if there are any process in process table, then calls SPIM_timerHandler() else system finishes. */
    case DELETE_PROCESS_SYSCALL:
      {
#ifndef _WIN32
      /* Closes the waitpid flag */
      if(read_mem_word(RUNNING_PROCESS_ADDRES)==read_mem_word(WAITPID_PROCESS)){
        set_mem_word(WAITPID_FLAG,0);
      }
      delete_process();
      printf("Delete Process Syscall Occured\n");
      /* If program done then finished the program */
      if(is_finish()){
        printf("Program Has Been Finished\n");
        force_break=true;
        spim_return_value = 0;
        return (0);
      }
      /* If program is not done then does context switch */
      else{
        if(read_mem_word(RUNNING_PROCESS_ADDRES)==0)
          set_mem_word(HANDLER_FLAG,0);
        SPIM_timerHandler();
      }
#endif
  break;
      }
    /* Waits for the A0's process to finish. */
    case WAITPID_SYSCALL:
      {
#ifndef _WIN32
      int addres=PCB_ADDRESS(R[REG_A0]);                                    /* Finds the input process's addres */
      set_mem_word(WAITPID_FLAG,1);                                         /* Sets waitpid flag to 1 */
      set_mem_word(WAITPID_PROCESS,R[REG_A0]);                              /* Sets waitpid process to input process */
      set_mem_word(WAITPID_END_ADDRES,read_mem_word(END_TEXT_PC(addres)));  /* Sets waitpid end addres to end of the program's text addres */
#endif
  break;
      }
    /* Returns the random integer value bounded by A0 register */
    case RNG_SYSCALL:
      {
#ifndef _WIN32
      int bound=R[REG_A0];         /* Bounded by $a0 */
      srand (time(NULL));          /* Gets the random number */
      R[REG_RES] = rand() % bound; /* Gets the modulo of random number according to bound */
#endif
  break;
      }
    /* Saves the machine's current state to ProcessTable[running_process] */
    case SAVE_THE_STATE:
      {
#ifndef _WIN32
      if(read_mem_word(HANDLER_FLAG)){
        int running_process=PCB_ADDRESS(read_mem_word(RUNNING_PROCESS_ADDRES));                 /* Finds the running process's addres in the assembly file (kernel). */
        printf("%s",(char*)mem_reference(CONTEXT_SWITCH_PRINT));
        write_string_addres("READY",PROCESS_STATE(running_process));                            /* Changes the process state */
        printf("Before the process change:\n");
        print_pcb_infos(read_mem_word(RUNNING_PROCESS_ADDRES));                                 /* Prints the current process's informations */
        if(read_mem_word(PC(running_process))!=0 && (unsigned int)read_mem_word(END_TEXT_PC(running_process))<=PC ){
          delete_process();                                                                     /* If there is overflow, then deletes the process */
        }
        if(read_mem_word(PC(running_process))!=0){                                              /* If current process is not empty then saves the current data to process table */ 
          set_mem_word(PC(running_process),PC);                                                 /* Saves all information into ProcessTable[running process] */
          set_mem_word(STACK_ADDRES(running_process),R[29]);
          write_string_addres("READY",PROCESS_STATE(running_process));
          register_to_memory(read_mem_word(RUNNING_PROCESS_ADDRES));
          set_mem_word(HI(running_process),HI);
          set_mem_word(LO(running_process),LO);
          set_mem_word(DATA_PC(running_process),current_data_pc());
          set_mem_word(END_TEXT_PC(running_process),current_text_pc());
        }
      }
#endif
      break;
      }
    /* Round Robin Scheduling */
    /* Finds the another not empty place in ProcessTable for change */
    case ROUND_ROBIN_SCHEDULING:
      {
#ifndef _WIN32
      int i=-1;
      if(read_mem_word(HANDLER_FLAG)){
        for(i=(read_mem_word(RUNNING_PROCESS_ADDRES)+1)%read_mem_word(PROCESS_TABLE_SIZE_ADDRES);i<read_mem_word(PROCESS_TABLE_SIZE_ADDRES);++i){
          if(read_mem_word(PC(PCB_ADDRESS(i)))!=0){                                             /* If not empty then breaks. */
            break;
          }
          if(i==read_mem_word(PROCESS_TABLE_SIZE_ADDRES)-1){                                    /* Restart to begin */
            i=-1;
          }
        }

        /* Waitpid syscall flag's handling */
        if(PC>=(unsigned int)read_mem_word(WAITPID_END_ADDRES)){  
          set_mem_word(WAITPID_FLAG,0);
        }
        if(read_mem_word(WAITPID_FLAG)==1){
          i=read_mem_word(WAITPID_PROCESS);
        }
      }
      R[REG_RES]=i;
#endif
      break;
      }
    /* Updates the current registers and machine informations by ProcessTable[$a0] process */
    case CHANGE_THE_PROCESS:
      {
#ifndef _WIN32
    int i=R[REG_A0];
    if(read_mem_word(HANDLER_FLAG)){
      /* Updates the current registers and machine informations by ProcessTable[i] process */
      char state[]="RUNNING";
      int new_address=PCB_ADDRESS(i);
      PC=read_mem_word(PC(new_address));
      R[29]=read_mem_word(STACK_ADDRES(new_address));
      memory_to_register(i);
      HI=read_mem_word(HI(new_address));
      LO=read_mem_word(LO(new_address));
      set_text_pc(read_mem_word(END_TEXT_PC(new_address)));
      set_data_pc(read_mem_word(DATA_PC(new_address)));
      write_string_addres(state,PROCESS_STATE(new_address));

      /* Updates the current running process to i */
      set_mem_word(RUNNING_PROCESS_ADDRES,i);

      printf("After the process change:\n");
      print_pcb_infos(read_mem_word(RUNNING_PROCESS_ADDRES));
    }
    set_mem_word(HANDLER_FLAG,1);
    R[REG_RES]=i;
#endif
      break;
      }

    default:
      run_error ("Unknown system call: %d\n", R[REG_V0]);
      break;
    }

#ifdef _WIN32
    windowsParameterHandlingControl(1);
#endif
  return (1);
}


void
handle_exception ()
{
  quiet=1;
  if (!quiet && CP0_ExCode != ExcCode_Int)
    error ("Exception occurred at PC=0x%08x\n", CP0_EPC);

  exception_occurred = 0;
  PC = EXCEPTION_ADDR;

  switch (CP0_ExCode)
    {
    case ExcCode_Int:
      break;

    case ExcCode_AdEL:
      if (!quiet)
	error ("  Unaligned address in inst/data fetch: 0x%08x\n", CP0_BadVAddr);
      break;

    case ExcCode_AdES:
      if (!quiet)
	error ("  Unaligned address in store: 0x%08x\n", CP0_BadVAddr);
      break;

    case ExcCode_IBE:
      if (!quiet)
	error ("  Bad address in text read: 0x%08x\n", CP0_BadVAddr);
      break;

    case ExcCode_DBE:
      if (!quiet)
	error ("  Bad address in data/stack read: 0x%08x\n", CP0_BadVAddr);
      break;

    case ExcCode_Sys:
      if (!quiet)
	error ("  Error in syscall\n");
      break;

    case ExcCode_Bp:
      exception_occurred = 0;
      return;

    case ExcCode_RI:
      if (!quiet)
	error ("  Reserved instruction execution\n");
      break;

    case ExcCode_CpU:
      if (!quiet)
	error ("  Coprocessor unuable\n");
      break;

    case ExcCode_Ov:
      if (!quiet)
	error ("  Arithmetic overflow\n");
      break;

    case ExcCode_Tr:
      if (!quiet)
	error ("  Trap\n");
      break;

    case ExcCode_FPE:
      if (!quiet)
	error ("  Floating point\n");
      break;

    default:
      if (!quiet)
	error ("Unknown exception: %d\n", CP0_ExCode);
      break;
    }
}
