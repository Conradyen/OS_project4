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

//========================= free list =======================
/**
 * newly defined struct freeListNode
 * use a queue to control free list
 * Ming-Hsuan
 */
/*
typedef struct{
  int free_frame;
  struct freeListNode next*;
}freeListNode;

void free_list_inti();
void insert_FLtail(int idx);
*/
//used to be int change to linked list node freeListNode
void freeList_init();
int freeFhead,freeFtail; // the head and tail of free frame list

//==========================================================

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
//=============== functions for address ======================

/**
 * Ming-Hsuan
 */

 int get_pagenum(int addr);//return page number
 int get_offset(int addr);//return off set

int get_pagenum(int addr){
  return (addr >> pagenumShift);
}

int get_offset(int addr){
  return (addr & pageoffsetMask);
}

//========================================================

int calculate_memory_address (unsigned offset, int rwflag)
{
  /**
   * Ming-Hsuan
   * @param NULL [description]
   */
  // rwflag is used to differentiate the caller
  // different access violation decisions are made for reader/writer
  // if there is a page fault, need to set the page fault interrupt
  // also need to set the age and dirty fields accordingly
  // returns memory address or mPFault or mError
  int pagenum = get_pagenum(offset);
  int framenum;
  if(pagenum < maxPpages){
    framenum = CPU.PTptr[pagenum];
		//memFrame[framenum].age >> 1;//zxm
		if(flagWrite == rwflag)	memFrame[framenum].dirty = dirtyFrame;//zxm
	  return ((pagenum << pagenumShift) | get_offset(offset));
  }
  // TODO mError
  else if(pagenum >= maxPpages){
    printf(">>>>>>>>>>>>>>> segmentation fault  <<<<<<<<<<<<<<<<<<<");
    return mError;
  }
  else{//page fault
    //set page fault interrupt
		set_interrupt (pFaultException);//zxm
    return mPFault;
  }
}

int get_data (int offset)
{
  /**
   * Ming-Hsuan
   * @param offset   [description]
   * @param flagRead [description]
   */
  // call calculate_memory_address to get memory address
  // copy the memory content to MBR
  // return mNormal, mPFault or mError
  int maddr = calculate_memory_address(offset,flagRead);
  //mPFault
  if(maddr == mPFault){
    return mPFault;
  }
  //mError
  else if(maddr == mError){
    return mError;
  }
  else{
    CPU.MBR = Memory[maddr].mData;
    return mNormal;
  }
}

int put_data (int offset)
{
  /**
   * Ming-Hsuan
   * @param offset    [description]
   * @param flagWrite [description]
   */
  // call calculate_memory_address to get memory address
  // copy MBR to memory
  // return mNormal, mPFault or mError
  int maddr = calculate_memory_address(offset,flagWrite);
  if(maddr == mPFault){
    return mPFault;
  }
  //mError
  else if(maddr == mError){
    return mError;
  }
  else{
    Memory[maddr].mData = CPU.AC;
    return mNormal;
  }
}

int get_instruction (int offset)
{
  /**
   * Ming-Hsuan
   * @param offset   [description]
   * @param flagRead [description]
   */
  int maddr, instr;
  // call calculate_memory_address to get memory address
  // convert memory content to opcode and operand
  // return mNormal, mPFault or mError
  maddr = calculate_memory_address(offset,flagRead);
  //page fault
  if(maddr == mPFault){
    return mPFault;
  }
  //mError
  else if(maddr == mError){
    return mError;
  }
  else{
    instr = Memory[maddr].mInstr;
    CPU.IRopcode = instr >> opcodeShift;
    CPU.IRoperand = instr & operandMask;
    return mNormal;
  }
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
{ int i;
  // dump the content of the memory (of frame findex)
  for(i = 0;i<pageSize;i++){
    int addr = findex*pageSize + i;
    printf("Memory frame index %d Data %.2f instruction %d ",findex,Memory[addr].mData,Memory[addr].mInstr);
  }
}

void dump_memory ()
{ int i;
  /**
   * ????????????????????????????
   * Ming-Hsuan
   */
  // dump all frames of the entire memory
  printf("*************** dump memory comtent ***************");
  for(i = 0;i < pageSize*numPages;i++){
    dump_one_frame(i);
  }
}

// above: dump memory content, below: only dump frame infor

void dump_free_list ()
{
  /**
   * dump free list
   * Ming-Hsuan
   */
  // dump the list of free memory frames
  printf("*************** dump free list ***************");
  int temp;
  temp = freeFhead;
  while(temp != nullIndex){
    printf("free frame : %d \n",temp);
    temp = memFrame[temp].next;
  }
  printf("\n");
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

  dump_free_list();
}

// when a frame is selected, its original process page is to be swapped out,
// and a new process page is to be swapped in: call this function
// to update the frame info for the new process page
void  update_newframe_info (findex, pid, page)
int findex, pid, page;
{
  /**
   * ZXM
   * @param findex [description]
   * @param status [description]
   */
  memFrame[findex].page = page;
  memFrame[findex].pid = pid;
 	memFrame[findex].free = usedFrame;
  //where is the new frame ??????????????
	memFrame[findex].next = nullIndex;
	memFrame[findex].prev = nullIndex;
}

// add a frame at the tail of the free list
// should write dirty frames to disk and remove them from process page table
// but we delay updates till the actual swap (page_fault_handler)
// unless frames are from the terminated process (status = nullPage)
// so, the process can continue using the page, till actual swap
void addto_free_frame (int findex, int status)
{ //add int pid
  int i ;
 	mType *buf;
  if(status == dirtyFrame){
    //write to disk
		for(i=0;i<pageSize;i++) {
			buf[i].mInstr = Memory[findex+i].mInstr;
			buf[i].mData = Memory[findex+i].mData;
		}
    //insert_swapQ (pid, page, buf, actWrite, pready)//zxm how do i know which pid it belong?
		if(freeFhead == nullIndex && freeFtail== nullIndex) { //??????
			freeFhead = findex; freeFtail = findex;
		} else {
			memFrame[freeFtail].next = findex;
			freeFtail = findex;
			memFrame[findex].free = freeFrame;
		}
  }else if(status == cleanFrame){
    //do add to free list
		if(freeFhead == nullIndex && freeFtail== nullIndex) {
			freeFhead = findex; freeFtail = findex;
		} else {
			memFrame[freeFtail].next = findex;
			freeFtail = findex;
			memFrame[findex].free = freeFrame;
		}
  }


}
int get_agest_frame(){
//zxm begin----------------
	int tmp_lowest = highestAge;
  int i,j,frmcnt;
	int agest_frame = numPages;
	//1st round -->> find out lowest age
	for (i=OSpages; i < numPages; i++){
    if(memFrame[i].age == 0){
      addto_free_frame(i,memFrame[i].dirty);
      return i;
  }
 		if(memFrame[i].age < tmp_lowest){
			tmp_lowest = memFrame[i].age;
      frmcnt = 1;
		} else if(memFrame[i].age == tmp_lowest){
			frmcnt += 1;
  	}
	}
	//2nd round -->> find out lowest age
  for(j = OSpages; j< numPages; j++){
    if(memFrame[i].age == tmp_lowest) {
      addto_free_frame(i,memFrame[i].dirty);
      return i;
  }
  //if there is only one lowst page which is dirty page, then do the swap out  /?????

//zxm end----------------
	}
}

// get a free frame from the head of the free list
// if there is no free frame, then get one frame with the lowest age
// this func always returns a frame, either from free list
int get_free_frame ()
{ int i;
  /**
   * Ming-Hsuan
   * @param freeFtail [description]
   */
//zxm begin----------------
 	if (freeFhead != nullIndex){
		int head = freeFhead;
		freeFhead = memFrame[freeFhead].next;
		return head;
	} else {
		return get_agest_frame();//?
	}
//zxm end----------------
}

//helper function
int _log(int num){
  /**
   * Ming-Hsuan
   * @param num [description]
   */
  //assume num is power of 2
  if(num == 1){
    return 1;
  }else{
    return 1+_log(num/2);
  }
}

//=================== free list related function ======================

// void free_list_init(){
//   /**
//    * initialize free list for simOS
//    * start for OSpages to numPages
//    * Ming-Hsuan
//    */
//   for(int i = OSpages;i<numPages;i++){
//     insert_Flist(i);
//   }
// }
//
// void insert_Flist(int idx){
//   /**
//    * insert frame to free list after pages is free
//    * Ming-Hsuan
//    */
//     freeListNode newNode = (freeListNode *) malloc(sizeof(freeListNode));
//     newNode.freeframe = idx;
//     newNode.next = NULL;
//     if (freeFtail == NULL) // head would be NULL also
//       { freeFtail = newNode; freeFhead = newNode; }
//     else // insert to tail
//       { freeFtail->next = newNode; freeFtail = newNode; }
//
// }

//=============================================================
void freeList_init(){
  int i;
  /**
   * assume memFrame has been initialized
   * Ming-Hsuan
   * @param i [description]
   */
  for(i = OSpages;i < numPages;i++){
    memFrame[i].age = zeroAge;
    memFrame[i].dirty = cleanFrame;
    memFrame[i].free = freeFrame;
    memFrame[i].pinned = nopinFrame;
    memFrame[i].pid = nullPid;
    if((i+1) < numPages){
      memFrame[i].next = i+1;
    }else{
      memFrame[i].next = nullIndex;
    }
    memFrame[i].prev = i-1;
  }
}

void initialize_memory ()
{ int i;
  /**
   * Mnig-Hauan
   */

  // create memory + create page frame array memFrame
  //Memory -> VM
  Memory = (mType *) malloc (numPages*pageSize*sizeof(mType));
  //Frame -> PM
  memFrame = (FrameStruct *) malloc (numPages*sizeof(FrameStruct));

  // compute #bits for page offset, set pagenumShift and pageoffsetMask
  // *** ADD CODE
  //page offset = log2(page size )
  //pageoffset -> pagesize
  pagenumShift = _log(pageSize);

  //pageoffsetMask shift pagenumShift
  //operandMask 0x00ffffff
  //shift operandMask by number of bits of page number
  pageoffsetMask = operandMask >> pagenumShift;
  //zxm begin----------------
  pageoffsetMask = 0xffffffff >> (32-pagenumShift);
  //zxm end----------------
  // initialize OS pages
  for (i=0; i<OSpages; i++)
  { //OS pages are not in free list
    memFrame[i].age = zeroAge;
    memFrame[i].dirty = cleanFrame;
    memFrame[i].free = usedFrame;
    memFrame[i].pinned = pinnedFrame;
    memFrame[i].pid = osPid;
  }
  // initilize the remaining pages, also put them in free list
  // *** ADD CODE

  freeList_init();//put in free list
  //set head and tail of memFrame linked list
  freeFhead = OSpages;
  freeFtail = numPages - 1;
}

//==========================================
// process page table manamgement
//==========================================

void init_process_pagetable (int pid)
{
  /**
   * Ming-Hsuan
   * @param maxPpages [description]
   */

  int i;

  PCB[pid]->PTptr = (int *) malloc (maxPpages);
  // initialize the page table for the process
  // *** ADD CODE
  //initialize all pages to null
  for(i = 0;i < maxPpages;i++){
    PCB[pid]->PTptr[i] = nullPage;
  }

}

// frame can be normal frame number or nullPage, diskPage
void update_process_pagetable (pid, page, frame)
int pid, page, frame;
{
  /**
   * ZXM
   * @param  pid [description]
   * @return     [description]
   */
  // update the page table entry for process pid to point to the frame
  // or point to disk or null
  //add nullPage
  //add disk page
  PCB[pid]->PTptr[page] = frame;
}

int free_process_memory(int pid)
{
  // free the memory frames for a terminated process
  // some frames may have already been freed, but still in process pagetable
	int framenum,i;
	for(i =0; i < maxPpages; i++)
	{
		framenum = PCB[pid]->PTptr[i];
		memFrame[framenum].age = zeroAge;
  	memFrame[framenum].dirty = cleanFrame;
  	memFrame[framenum].free = freeFrame;
  	memFrame[framenum].pinned = nopinFrame;
  	memFrame[framenum].pid = nullPid;
	}
  return mNormal;
}

void dump_process_pagetable (int pid)
{ int i;
  /**
   * Ming-Hsuan
   * @param Pid [description]
   * @param pid [description]
   */
  // print page table entries of process pid
  printf("=================== dump process page table Pid = %d",pid);
  for(i = 0;i < maxPpages;i++){
    printf(" %d \n",PCB[pid]->PTptr[i]);
  }
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

//
// void select_aged_page(){
//
// }

int page_fault_handler ()
{
  /**
   * Ming-Hsuan
   */
  int swappid, swappage, dirty;
  int *inbuf, *outbuf;
  // handle page fault
  // obtain a free frame or get a frame with the lowest age
  // if the frame is dirty, insert a write request to swapQ
  // insert a read request to swapQ to bring the new page to this frame
  // update the frame metadata, the page tables of the processes
  swappid = -1;
  //get free frame always returns a frame
  swappage = get_free_frame();

  if (&swappid < 0) return (mError);

  // update page table
  //=============================================
  //TO BE changed
  //inbuf = get_memoryPtr (pid, page);
  //outbuf = get_memoryPtr (swappid, swappage);
  // if (!dirty)  // no need to write back
  //   { swappid = -1; swappage = -1; }
  // insert_swapQ (pid, page, inbuf, swappid, swappage, outbuf);
}

// scan the memory and update the age field of each frame
void memory_agescan ()
{ int i;
  /**
   * Ming-Hsuan
   * @param i [description]
   */
  for(i = OSpages;i < numPages;i++){
    memFrame[i].age = memFrame[i].age >> 1;
    if(memFrame[i].age == 0x0000000){
      /**
       * add to free list when age is 0x0000000
       */
      addto_free_frame(i,memFrame[i].dirty);
    }
  }
}

void initialize_memory_manager ()
{ initialize_memory ();
  add_timer (periodAgeScan, osPid, actAgeInterrupt, periodAgeScan);
}
