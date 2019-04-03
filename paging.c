#include <stdio.h>
#include <stdlib.h>
#include "simos.h"

//======================================================================
// Our memory addressing is in WORDs, because of the Memory structure def
// - so all addressing is computed in words
//======================================================================


// Memory definitions, including the memory itself and a page structure
// that maintains the informtion about each memory page
// config.sys input: pageSize, numPages, OSpages
// ------------------------------------------------
// process page table definitions
// config.sys input: loadPpages, maxPpages

// need to be consistent with dataSize in simos.h

mType *Memory;   // The physical memory, size = pageSize*numPages

typedef unsigned ageType;
typedef struct
{ int pid, page;   // the frame is allocated to process pid for page page
  ageType age;
  char free, dirty, pinned;   // in real systems, these are bits
  int next, prev;
} FrameStruct;

FrameStruct *memFrame;   // memFrame[numPages]
int freeFhead, freeFtail;   // the head and tail of free frame list

// define special values for page/frame number
#define nullIndex -1   // free frame list null pointer
#define nullPage -1   // page does not exist yet
#define diskPage -2   // page is on disk swap space
#define pendingPage -3  // page is pending till it is actually swapped
   // have to ensure: #memory-frames < address-space/2, (pageSize >= 2)
   //    becuase we use negative values with the frame number
   // nullPage & diskPage are used in process page table
   // nullPage & pendingPage are used for status in addto_free_frame

// define values for fields in FrameStruct
#define zeroAge 0x01000000
#define highestAge 0x80000000
#define dirtyFrame 1
#define cleanFrame 0
#define freeFrame 1
#define usedFrame 0
#define pinnedFrame 1
#define nopinFrame 0

// define shifts and masks for instruction and memory address
#define opcodeShift 24
#define operandMask 0x00ffffff

// shift address by pagenumShift bits to get the page number
unsigned pageoffsetMask;
int pagenumShift; // 2^pagenumShift = pageSize

#define itemPerLine 8  // for printing, define items per line

// define the information structure for page fault exception interrupt
// set in calculate_address when interrupting, used by page_fault_handler
// page_fault_handler is called immediately after get_data/instruction
// so there is no way that the PFexcepInfo will be lost
typedef struct
{ int pid;
  int page;
} PFexcepInfoStruct;
PFexcepInfoStruct PFexcepInfo;

void print_one_frameinfo (int indx);

//============================
// Our memory implementation is a mix of memory manager and physical memory.
// get_instr, put_instr, get_data, put_data are the physical memory operations
//   for instr, instr is fetched into registers: IRopcode and IRoperand
//   for data, data is fetched into registers: MBR (need to retain AC value)
//             but stored directly from AC
//   -- one reason is because instr and data do not have the same types
//      also for convenience
// allocate_memory, deallocate_memory are pure memory manager activities
//============================


//==========================================
// run time memory access operations, called by cpu.c
//==========================================

// define rwflag to indicate whehter the addr computation is for read or write
#define flagRead 1
#define flagWrite 2

// address calcuation are performed for the program in execution
// so, we can get address related infor from CPU registers
int calculate_memory_address (unsigned offset, int rwflag)
{
  // rwflag is used to differentiate the caller
  // different access violation decisions are made for reader/writer
  // if there is a page fault, need to set the page fault interrupt
  // also need to set the age and dirty fields accordingly
  // returns memory address or mPFault or mError
}

int get_data (int offset)
{
  // call calculate_memory_address to get memory address
  // copy the memory content to MBR
  // return mNormal, mPFault or mError
  }
}

int put_data (int offset)
{
  // call calculate_memory_address to get memory address
  // copy MBR to memory
  // return mNormal, mPFault or mError
}

int get_instruction (int offset)
{
  // call calculate_memory_address to get memory address
  // convert memory content to opcode and operand
  // return mNormal, mPFault or mError
}

// these two direct_put functions are only called for loading idle process
// no specific protection check is done
void direct_put_instruction (int findex, int offset, int instr)
{ int addr = (offset & pageoffsetMask) | (findex << pagenumShift);
  Memory[addr].mInstr = instr;
}

void direct_put_data (int findex, int offset, mdType data)
{ int addr = (offset & pageoffsetMask) | (findex << pagenumShift);
  Memory[addr].mData = data;
}

//==========================================
// Memory and memory frame management
//==========================================

void dump_one_frame (int findex)
{
  // dump the content of the memory (of frame findex)
}

void dump_memory ()
{
  // dump all frames of the entire memory
}

// above: dump memory content, below: only dump frame infor

void dump_free_list ()
{
  // dump the list of free memory frames
}

void print_one_frameinfo (int indx)
{ printf ("pid/page/age=%d,%d,%x, ",
          memFrame[indx].pid, memFrame[indx].page, memFrame[indx].age);
  printf ("dir/free/pin=%d/%d/%d, ",
          memFrame[indx].dirty, memFrame[indx].free, memFrame[indx].pinned);
  printf ("next/prev=%d,%d\n",
          memFrame[indx].next, memFrame[indx].prev);
}

void dump_memoryframe_info ()
{
  // print the frame info of all frames
  // call dump_free_list () to print the free list
}

// when a frame is selected, its original process page is to be swapped out,
// and a new process page is to be swapped in: call this function
// to update the frame info for the new process page
void  update_newframe_info (findex, pid, page)
int findex, pid, page;
{
}

// add a frame at the tail of the free list
// should write dirty frames to disk and remove them from process page table
// but we delay updates till the actual swap (page_fault_handler)
// unless frames are from the terminated process (status = nullPage)
// so, the process can continue using the page, till actual swap
void addto_free_frame (int findex, int status)
{
}

// get a free frame from the head of the free list
// if there is no free frame, then get one frame with the lowest age
// this func always returns a frame, either from free list
int get_free_frame ()
{

}

void initialize_memory ()
{ int i;

  // create memory + create page frame array memFrame
  Memory = (mType *) malloc (numPages*pageSize*sizeof(mType));
  memFrame = (FrameStruct *) malloc (numPages*sizeof(FrameStruct));

  // compute #bits for page offset, set pagenumShift and pageoffsetMask
  // *** ADD CODE

  // initialize OS pages
  for (i=0; i<OSpages; i++)
  { memFrame[i].age = zeroAge;
    memFrame[i].dirty = cleanFrame;
    memFrame[i].free = usedFrame;
    memFrame[i].pinned = pinnedFrame;
    memFrame[i].pid = osPid;
  }
  // initilize the remaining pages, also put them in free list
  // *** ADD CODE

}

//==========================================
// process page table manamgement
//==========================================

void init_process_pagetable (int pid)
{ int i;

  PCB[pid]->PTptr = (int *) malloc (addrSize*maxPpages);

  // initialize the page table for the process
  // *** ADD CODE
  
}

// frame can be normal frame number or nullPage, diskPage
void update_process_pagetable (pid, page, frame)
int pid, page, frame;
{
  // update the page table entry for process pid to point to the frame
  // or point to disk or null
}

int free_process_memory (int pid)
{
  // free the memory frames for a terminated process
  // some frames may have already been freed, but still in process pagetable
}

void dump_process_pagetable (int pid)
{
  // print page table entries of process pid
}

void dump_process_memory (int pid)
{
  // print out the memory content for process pid
}

//==========================================
// the major functions for paging, invoked externally
//==========================================

// update PT of pidin: can be done before the page is swapped in
// because pidin is in IO wait state and will not be executed
// -------------------------------------------------------------
// update PT of pidout, which lost frame fff for its page ppp:
// update has to be done now, so that if the process is in execution
// during swap IO, its page table for ppp correctly points to disk
// ----------
// In case the process needs to reference ppp, but fff is not copied
// to the swap space yet, it is fine, because the request to bring in
// ppp will be after the request for writing fff

#define sendtoReady 1  // has to be the same as those in swap.c
#define notReady 0
#define actRead 0
#define actWrite 1

void page_fault_handler ()
{
  // handle page fault
  // obtain a free frame or get a frame with the lowest age
  // if the frame is dirty, insert a write request to swapQ
  // insert a read request to swapQ to bring the new page to this frame
  // update the frame metadata, the page tables of the processes
}

// scan the memory and update the age field of each frame
void memory_agescan ()
{
}

void initialize_memory_manager ()
{ initialize_memory ();
  add_timer (periodAgeScan, osPid, actAgeInterrupt, periodAgeScan);
}
