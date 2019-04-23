#include <stdio.h>
#include <stdlib.h>
#include "simos.h"

// need to be consistent with paging.c: mType and constant definitions
#define opcodeShift 24
#define operandMask 0x00ffffff
#define diskPage -2

FILE *progFd;

//==========================================
// load program into memory and build the process, called by process.c
// a specific pid is needed for loading, since registers are not for this pid
//==========================================

// may return progNormal or progError (if the program is incorrect)
int load_instruction (mType *buf, int page, int offset)
{
  // load instruction to buffer
  int opcode,operand;
  int addr = (page * pageSize)+offset;
  fscanf (progFd, "%d %d\n", &opcode, &operand);
  if (Debug) printf ("load instruction: %d, %d\n",opcode, operand);//zxm
  opcode = opcode << opcodeShift;
  operand = operand & operandMask;
  buf[addr].mInstr = opcode | operand;
  return (progNormal);//zxm mNormal); how to generate progError
}

int load_data (mType *buf, int page, int offset)
{
  // load data to buffer (same as load instruction, but use the mData field
  int data;
  int addr = (page * pageSize)+offset;
  fscanf (progFd, "%d\n", &data);
  if (Debug) printf ("load data: %d\n", data);//zxm
  buf[addr].mData = data;
  return (progNormal);//zxm mNormal); how to generate progError
}

// load program to swap space, returns the #pages loaded
int load_process_to_swap (int pid, char *fname)
{
  mType *buf;
  int msize, numinstr, numdata;
	int page,offset,numpage;//zxm
  int ret,i;//check return value
  //NOTE ***************
  //int ret  = something; are not valid in std=c99
  progFd = fopen(fname,"r");
  //printf("file %s open secessfully!!!\n",fname);
  if (progFd == NULL)
  { printf ("Incorrect program name: %s!\n", fname);
    return;
  }
  ret = fscanf (progFd, "%d %d %d\n", &msize, &numinstr, &numdata);
  //printf("first line\n");
  if (ret < 3)   // did not get all three inputs
  { printf ("Submission failure: missing %d program parameters!\n", 3-ret);
    return;
  }
  numpage = numinstr / pageSize + numdata / pageSize;
  // read from program file "fname" and call load_instruction & load_data
  // to load the program into the buffer, write the program into
  // swap space by calling write_swap_page (pid, page, buf)
  // update the process page table to indicate that the page is not empty
  // and it is on disk
  //=====================================================================
  //TOBE changed
  for (i=0; i<numinstr; i++)
  {	page = i / pageSize;
    offset = i % pageSize;
    //if (Debug) printf ("Process %d loading Line %d\n", pid, i);
    ret = load_instruction (buf,page,offset);
    if (ret == progNormal) write_swap_page(pid,page,buf);//zxm
    if (ret == progError) { PCB[pid]->exeStatus = eError; return;  	 } //???
  }
  for (i = numinstr; i < msize; i++)
  { page = i / pageSize;
    offset = i % pageSize;
    //if (Debug) printf ("Process %d loading Line %d\n", pid, i);
    ret = load_data (buf, page, offset);
    if (ret == progNormal) write_swap_page(pid,page,buf);//zxm
    if (ret == progError) { PCB[pid]->exeStatus = eError; return; } //???
  }
  //=====================================================================
  return numpage;
}

int load_pages_to_memory (int pid, int numpage)
{
  // call insert_swapQ to load the pages of process pid to memory
  // #pages to load = min (loadPpages, numpage = #pages loaded to swap for pid)
  // ask swap.c to place the process to ready queue only after the last write
  mType *buf;//zxm
  int i;
  pringf("here in load_pages_to_memor\n");
	for(i=0;i<numpage;i++) {
    read_swap_page(pid,i,buf);
  	insert_swapQ (pid, i, buf, 0, 1);//code from swap to memory
	}

}

int load_process (int pid, char *fname)
{ int ret;

  ret = load_process_to_swap (pid, fname);   // return #pages loaded
  if (ret > 0)
  { ret = load_pages_to_memory (pid, ret); }
  return (ret);
}

// load idle process, idle process uses OS memory
// We give the last page of OS memory to the idle process
#define OPifgo 5   // has to be consistent with cpu.c
void load_idle_process ()
{ int page, frame;
  int instr, opcode, operand, data;

  init_process_pagetable (idlePid);
  page = 0;   frame = OSpages - 1;
  update_process_pagetable (idlePid, page, frame);
  update_newframe_info (frame, idlePid, page);

  // load 1 ifgo instructions (2 words) and 1 data for the idle process
  opcode = OPifgo;   operand = 0;
  instr = (opcode << opcodeShift) | operand;
  direct_put_instruction (frame, 0, instr);   // 0,1,2 are offset
  direct_put_instruction (frame, 1, instr);
  direct_put_data (frame, 2, 1);
}
