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
#include <queue>
#include <time.h>
#include <stdlib.h>

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

/* Process Control Block */
struct process_control_block 
{ 
  int ProcessID; 
  char ProcessName[100]; 
  mem_addr Programcounter; 
  mem_addr text_pc;
  mem_addr end_text_pc;
  mem_addr data_pc;
  mem_addr StackPointerAddress;
  char ProcessState[50];
  int parent_process;

  reg_word R_temp[R_LENGTH];
  reg_word CCR_temp[4][32],CPR_temp[4][32];
  reg_word HI_temp,LO_temp;
};

/* holds the current process id */
int running_process=0;

/* Process table */
struct process_control_block* ProcessTable[50]={NULL};

/* process counts */
int all_process_count=0;
int running_process_count=0;

/* waitpid parameters */
bool waitpid_flag=false;
int waitpid_process=-1;
mem_addr waitpid_end_addres=0;

/* handler_flag that checks whether interrupt is working. */
bool handler_flag=true;


/* Process table printer */
void print_process(int i){
  cout<<dec<<"Process ID:"<<ProcessTable[i]->ProcessID<<endl;
  cout<<"Process Name:"<<ProcessTable[i]->ProcessName<<endl;
  cout<<hex<<"Current PC:"<<ProcessTable[i]->Programcounter<<endl;
  cout<<hex<<"Start Text PC:"<<ProcessTable[i]->text_pc<<endl;
  cout<<hex<<"End Text PC:"<<ProcessTable[i]->end_text_pc<<endl;
  cout<<hex<<"Data PC:"<<ProcessTable[i]->data_pc<<endl;
  cout<<hex<<"Stack Pointer Address:"<<ProcessTable[i]->StackPointerAddress<<endl;
  cout<<dec<<"Process Table Size:"<<running_process_count<<endl;
  cout<<dec<<"Parent Process:"<<ProcessTable[i]->parent_process<<endl;
  cout<<"Process State:"<<ProcessTable[i]->ProcessState<<endl<<endl;
}

/* Memory printer */
void print_memory(){
  cout<<"PRINT MEMORY"<<endl;
  cout<<hex<<"PC:"<<PC<<endl;
  cout<<hex<<"text_PC:"<<current_text_pc()<<endl;
  cout<<hex<<"data_PC:"<<current_data_pc()<<endl;
  cout<<hex<<"StackPointerAddress:"<<R[29]<<endl;
}

/* Only copies the given array in parameters  */
template <class T>
void carry_registers(T* copy,T* array,int size){ 
  for(int i=0;i<size;++i)
    copy[i]=array[i];
}

/* Only copies the given array in parameters  */
template <class T>
void carry_registers2d(T copy[][R_LENGTH],T array[][R_LENGTH],int size1,int size2){ 
  for(int i=0;i<size1;++i)
    for(int j=0;j<size2;++j)
      copy[i][j]=array[i][j];
}

/* Copies memory to process */
void memory_to_process(int pid){
  carry_registers<reg_word>(ProcessTable[pid]->R_temp,R,R_LENGTH);
  carry_registers2d<reg_word>(ProcessTable[pid]->CPR_temp,CPR,4,32);
  carry_registers2d<reg_word>(ProcessTable[pid]->CCR_temp,CCR,4,32);
  ProcessTable[pid]->Programcounter=PC;
  ProcessTable[pid]->text_pc=current_text_pc();
  ProcessTable[pid]->data_pc=current_data_pc();
  ProcessTable[pid]->HI_temp=HI;
  ProcessTable[pid]->LO_temp=LO;
  ProcessTable[pid]->StackPointerAddress=R[29];
}

/* Deletes the current process */
void delete_process(){
  delete(ProcessTable[running_process]);
  ProcessTable[running_process]= NULL;
  --running_process_count;
  cout<<"Process "<< running_process <<" deleted. After the delete process count: "<<running_process_count<<endl;
}

/* Controls is finished */
bool is_finish(){
  return running_process_count == 0 ? true : false;
}

/* Handles the interrupt and saves the current process and After that changes the process */
void SPIM_timerHandler()
{
  if(handler_flag){
    cout<<endl<<"**SPIM_timerHandler**"<<endl;
    if(ProcessTable[running_process]!=NULL && ProcessTable[running_process]->end_text_pc<=PC){
      delete_process();
    }
    /* Saves the current memory infos to process table's process */
    if(ProcessTable[running_process]!=NULL){
      ProcessTable[running_process]->Programcounter=PC;
      ProcessTable[running_process]->StackPointerAddress=R[29];
      strcpy(ProcessTable[running_process]->ProcessState,"READY");
      carry_registers(ProcessTable[running_process]->R_temp,R,32);
      carry_registers2d(ProcessTable[running_process]->CCR_temp,CCR,4,32);
      carry_registers2d(ProcessTable[running_process]->CPR_temp,CPR,4,32);
      ProcessTable[running_process]->HI_temp=HI;
      ProcessTable[running_process]->LO_temp=LO;
      ProcessTable[running_process]->data_pc=current_data_pc();
      ProcessTable[running_process]->end_text_pc=current_text_pc();
    }
    cout<<"Before change current process: "<<running_process<<endl;

    /* Finds the another not NULL place in ProcessTable for change */
    int i;
    for(i=running_process+1;i<50;++i){
      if(ProcessTable[i]!=NULL){
        break;
      }
      if(i==49){
        i=-1;
      }
    }

    /* Waitpid conditions */
    if(PC>=waitpid_end_addres){
      waitpid_flag=false;
    }
    if(waitpid_flag){
      i=waitpid_process;
    }

    cout<<"After change current process: "<<i<<endl;
    print_process(i);

    /* Copies process table's process infos to current memory. */
    PC=ProcessTable[i]->Programcounter;
    R[29]=ProcessTable[i]->StackPointerAddress;
    carry_registers(R,ProcessTable[i]->R_temp,32);
    carry_registers2d(CCR,ProcessTable[i]->CCR_temp,4,32);
    carry_registers2d(CPR,ProcessTable[i]->CPR_temp,4,32);
    HI=ProcessTable[i]->HI_temp;
    LO=ProcessTable[i]->LO_temp;
    set_text_pc(ProcessTable[i]->end_text_pc);
    set_data_pc(ProcessTable[i]->data_pc);
    strcpy(ProcessTable[i]->ProcessState,"RUNNING");
    
    running_process=i;
  }
  handler_flag=true;
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
  if(ProcessTable[running_process]!=NULL && (ProcessTable[running_process]->end_text_pc<=PC)){
    R[REG_V0]=PROCESS_EXIT_SYSCALL;
  }

  /* Syscalls for the source-language version of SPIM.  These are easier to
     use than the real syscall and are portable to non-MIPS operating
     systems. */

  /* I add the BLOCKED and RUNNING state changes in I/O syscalls. I have added the new controls in the print int and print string syscalls to avoid runtime errors. */
  switch (R[REG_V0])
    {
    case PRINT_INT_SYSCALL:
      if(ProcessTable[running_process]!=NULL)
        strcpy(ProcessTable[running_process]->ProcessState,"BLOCKED");
      if(R[REG_A0]>=0x10000000){
        write_output (console_out, "%s",mem_reference (R[REG_A0]));
      }
      else{
        write_output (console_out, "%d", R[REG_A0]);
      }
      if(ProcessTable[running_process]!=NULL)
          strcpy(ProcessTable[running_process]->ProcessState,"RUNNING");
      break;
    case PRINT_FLOAT_SYSCALL:
      {
	float val = FPR_S (REG_FA0);
  if(ProcessTable[running_process]!=NULL)
    strcpy(ProcessTable[running_process]->ProcessState,"BLOCKED");
	write_output (console_out, "%.8f", val);
  if(ProcessTable[running_process]!=NULL)
    strcpy(ProcessTable[running_process]->ProcessState,"RUNNING");
	break;
      }

    case PRINT_DOUBLE_SYSCALL:
      //cout<<"print_double";
      if(ProcessTable[running_process]!=NULL)
        strcpy(ProcessTable[running_process]->ProcessState,"BLOCKED");
      write_output (console_out, "%.18g", FPR[REG_FA0 / 2]);
      if(ProcessTable[running_process]!=NULL)
        strcpy(ProcessTable[running_process]->ProcessState,"RUNNING");
      break;

    case PRINT_STRING_SYSCALL:
      if(ProcessTable[running_process]!=NULL)
          strcpy(ProcessTable[running_process]->ProcessState,"BLOCKED");
      if(0x10000000<=R[REG_A0]){
        write_output (console_out, "%s", mem_reference (R[REG_A0]));
      }
      else{
        write_output (console_out, "%d ", R[REG_A0]);
      }
      if(ProcessTable[running_process]!=NULL)
          strcpy(ProcessTable[running_process]->ProcessState,"RUNNING");
      break;

    case READ_INT_SYSCALL:
      {
	static char str [256];
  if(ProcessTable[running_process]!=NULL)
    strcpy(ProcessTable[running_process]->ProcessState,"BLOCKED");
	read_input (str, 256);
	R[REG_RES] = atol (str);
  if(ProcessTable[running_process]!=NULL)
    strcpy(ProcessTable[running_process]->ProcessState,"RUNNING");
	break;
      }

    case READ_FLOAT_SYSCALL:
      {
	static char str [256];
  if(ProcessTable[running_process]!=NULL)
    strcpy(ProcessTable[running_process]->ProcessState,"BLOCKED");
	read_input (str, 256);
	FPR_S (REG_FRES) = (float) atof (str);
  if(ProcessTable[running_process]!=NULL)
    strcpy(ProcessTable[running_process]->ProcessState,"RUNNING");
	break;
      }

    case READ_DOUBLE_SYSCALL:
      {
	static char str [256];
  if(ProcessTable[running_process]!=NULL)
    strcpy(ProcessTable[running_process]->ProcessState,"BLOCKED");
	read_input (str, 256);
	FPR [REG_FRES] = atof (str);
  if(ProcessTable[running_process]!=NULL)
    strcpy(ProcessTable[running_process]->ProcessState,"RUNNING");
	break;
      }

    case READ_STRING_SYSCALL:
      {
  if(ProcessTable[running_process]!=NULL)
    strcpy(ProcessTable[running_process]->ProcessState,"BLOCKED");
	read_input ( (char *) mem_reference (R[REG_A0]), R[REG_A1]);
	data_modified = true;
  if(ProcessTable[running_process]!=NULL)
    strcpy(ProcessTable[running_process]->ProcessState,"RUNNING");
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
      if(ProcessTable[running_process]!=NULL)
        strcpy(ProcessTable[running_process]->ProcessState,"BLOCKED");
      write_output (console_out, "%c", R[REG_A0]);
      if(ProcessTable[running_process]!=NULL)
        strcpy(ProcessTable[running_process]->ProcessState,"RUNNING");
      break;

    case READ_CHARACTER_SYSCALL:
      {
	static char str [2];
  if(ProcessTable[running_process]!=NULL)
    strcpy(ProcessTable[running_process]->ProcessState,"BLOCKED");
	read_input (str, 2);
	if (*str == '\0') *str = '\n';      /* makes xspim = spim */
	R[REG_RES] = (long) str[0];
  if(ProcessTable[running_process]!=NULL)
    strcpy(ProcessTable[running_process]->ProcessState,"RUNNING");
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
    /* Initialize the ProcessTable and creates the initial process that has 0 process id and names "init". */
    case INIT_SYSCALL:
      {
#ifndef _WIN32
	if(all_process_count==0){
    ProcessTable[0] = new process_control_block();
    memory_to_process(0);
    ProcessTable[0]->ProcessID=0;
    strcpy(ProcessTable[0]->ProcessName,"init");
    strcpy(ProcessTable[0]->ProcessState,"RUNNING");
    ProcessTable[0]->parent_process=-1;
    ProcessTable[0]->end_text_pc=current_text_pc();
    ProcessTable[0]->text_pc=0x00400020;
    ProcessTable[0]->Programcounter=PC+4;
  }
  ++all_process_count;
  ++running_process_count;
  running_process=0;

#endif
	break;
      }
    /* Allocates the new allocation in process table for the new process then fills it by current process'es copy. */
    case FORK_SYSCALL:
      {
#ifndef _WIN32
  int i;
  for(i=0;i<50;++i){
    if(ProcessTable[i]==NULL){
      ProcessTable[i]=new process_control_block();
      break;
    }
  }
  /* Filling the current process to new process table's index */
  char temp[20];
  strcpy(temp,ProcessTable[running_process]->ProcessName);
  strcpy(ProcessTable[i]->ProcessName,strcat(temp," child"));
  ProcessTable[i]->ProcessID=all_process_count;
  ProcessTable[i]->Programcounter=PC;
  ProcessTable[i]->text_pc=ProcessTable[running_process]->text_pc;
  ProcessTable[i]->end_text_pc=ProcessTable[running_process]->end_text_pc;
  ProcessTable[i]->data_pc=ProcessTable[running_process]->data_pc;
  ProcessTable[i]->StackPointerAddress=R[29];
  strcpy(ProcessTable[i]->ProcessState,"READY");
  ProcessTable[i]->parent_process=ProcessTable[running_process]->ProcessID;
  carry_registers(ProcessTable[i]->R_temp,R,32);
  carry_registers2d<reg_word>(ProcessTable[i]->CPR_temp,CPR,4,32);
  carry_registers2d<reg_word>(ProcessTable[i]->CCR_temp,CCR,4,32);
  ProcessTable[i]->HI_temp=HI;
  ProcessTable[i]->LO_temp=LO;

  ++all_process_count;
  ++running_process_count;

  R[REG_RES]=i;
#endif
	break;
      }
    /* Loads the file in the memory and changes the current process'es informations to loaded process */
    case EXECVE_SYSCALL:
      {
#ifndef _WIN32

  if(ProcessTable[running_process_count-1]->ProcessID!=0 && strcmp(ProcessTable[running_process_count-1]->ProcessName,"init")!=0){
    char* file_name=(char *)mem_reference (R[REG_A0]);
    mem_addr start_text_addr=current_text_pc();

    read_assembly_file(file_name);

    memory_to_process(running_process_count-1);

    strcpy(ProcessTable[running_process_count-1]->ProcessName,file_name);
    ProcessTable[running_process_count-1]->Programcounter=start_text_addr;
    ProcessTable[running_process_count-1]->text_pc=start_text_addr;
    ProcessTable[running_process_count-1]->end_text_pc=current_text_pc(); 
    ProcessTable[running_process_count-1]->data_pc=current_data_pc();

    /*if(ProcessTable[running_process_count-1]->end_text_pc>end_of_the_program){
      end_of_the_program=ProcessTable[running_process_count-1]->end_text_pc;
      end_process=running_process_count-1;
    }*/
  }

  R[REG_RES]=running_process_count-1;
#endif
	break;
      }
    /* Controls the process if there is no process in process table, then system finishes. */
    case PROCESS_EXIT_SYSCALL:
      {
#ifndef _WIN32
      if(is_finish()){
        cout<<"Program has been finished"<<endl;
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
      if(running_process==waitpid_process)
        waitpid_flag=false;
      delete_process();
      cout<<"Delete Process Syscall Occured"<<endl;
      if(is_finish()){
        cout<<"Program has been finished"<<endl;
        force_break=true;
        spim_return_value = 0;
        return (0);
      }
      else{
        if(running_process==0)
          handler_flag=false;
        SPIM_timerHandler();
      }
#endif
  break;
      }
    /* Waits for the A0's process to finish. */
    case WAITPID_SYSCALL:
      {
#ifndef _WIN32
      waitpid_flag=true;
      waitpid_process=R[REG_A0];
      waitpid_end_addres=ProcessTable[R[REG_A0]]->end_text_pc;
#endif
  break;
      }
    /* Returns the random integer value bounded by A0 register */
    case RNG_SYSCALL:
      {
#ifndef _WIN32
      int bound=R[REG_A0];
      srand (time(NULL));
      R[REG_RES] = rand() % bound;
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
