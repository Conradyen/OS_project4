#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <semaphore.h>
#include "simos.h"


//======================================================================
// This module handles swap space management.
// It has the simulated disk and swamp manager.
// First part is for the simulated disk to read/write pages.
//======================================================================
// Note: do not use swap content dump functions outside swap,
// It will cause lseek to move the file pointer during concurrent exeuction
// and cause errors to read/write functions
// Only access the dump swapQ functions externallyy

#define swapFname "swap.disk"
#define itemPerLine 8
int diskfd;
int swapspaceSize;
int PswapSize;
int pagedataSize;

sem_t swap_semaq;
sem_t swapq_mutex;
sem_t disk_mutex;

//===================================================
// This is the simulated disk, including disk read, write, dump.
// The unit is a page
//===================================================
// each process has a fix-sized swap space, its page count starts from 0
// first 2 processes: OS=0, idle=1, have no swap space (not true in real os)

int read_swap_page (int pid, int page, unsigned *buf)
{ 
  // reference the previous code for this part
  // but previous code was not fully completed
}

int write_swap_page (int pid, int page, unsigned *buf)
{ 
  // reference the previous code for this part
  // but previous code was not fully completed
}

int dump_process_swap_page (int pid, int page)
{ 
  // reference the previous code for this part
  // but previous code was not fully completed
}

void dump_process_swap (int pid)
{ 
  printf ("****** Dump swap pages for process %d\n", pid);
  for (j=0; j<maxPpages; j++) dump_process_swap_page (pid, j);
}

// open the file with the swap space size, initialize content to 0
initialize_swap_space ()
{ int ret, i, j, k;
  int buf[pageSize];

  swapspaceSize = maxProcess*maxPpages*pageSize*dataSize;
  PswapSize = maxPpages*pageSize*dataSize;
  pagedataSize = pageSize*dataSize;

  diskfd = open (swapFname, O_RDWR | O_CREAT, 0600);
  if (diskfd < 0) { perror ("Error open: "); exit (-1); }
  ret = lseek (diskfd, swapspaceSize, SEEK_SET); 
  if (ret < 0) { perror ("Error lseek in open: "); exit (-1); }
  for (i=2; i<maxProcess; i++)
    for (j=0; j<maxPpages; j++)
    { for (k=0; k<pageSize; k++) buf[k]=0;
      write_swap_page (i, j, buf);
    }
    // last parameter is the origin, offset from the origin, which can be:
    // SEEK_SET: 0, SEEK_CUR: from current position, SEEK_END: from eof
}


//===================================================
// Here is the swap space manager. 
//===================================================
// When a process address to be read/written is not in the memory,
// meory raises a page fault and process it (in kernel mode).
// We implement this by cheating a bit.
// We do not perform context switch right away and switch to OS.
// We simply let OS do the processing.
// OS decides whether there is free memory frame, if so, use one.
// If no free memory, then call select_aged_page to free up memory.
// In either case, proceed to insert the page fault req to swap queue
// to let the swap manager bring in the page
//===================================================

typedef struct SwapQnodeStruct
{ int pid, page, act, pready;
  unsigned *buf;
  struct SwapQnodeStruct *next;
} SwapQnode;
// pidin, pagein, inbuf: for the page with PF, needs to be brought in
// pidout, pageout, outbuf: for the page to be swapped out
// if there is no page to be swapped out (not dirty), then pidout = nullPid
// inbuf and outbuf are the actual memory page content

SwapQnode *swapQhead = NULL;
SwapQnode *swapQtail = NULL;

#define sendtoReady 1  // flags for pready field, indicate whehter to
#define notReady 0   // send the process to ready queue at the end
#define actRead 0   // flags for act (action), read or write
#define actWrite 1 

void print_one_swapnode (SwapQnode *node)
{ printf ("pid,page=(%d,%d), act,ready=(%d, %d), buf=%x\n", 
           node->pid, node->page, node->act, node->pready, node->buf);
}

void dump_swapQ ()
{ 
  // dump all the nodes in the swapQ
}

// act can be actRead or actWrite
// pready indicates whether to put the process to ready queue after swap
// pready can be sendtoReady (back to ready queue) or notReady
void insert_swapQ (pid, page, buf, act, pready)
int pid, page, act, pready;
unsigned *buf;
{ 
}

void process_one_swap ()
{ 
  // get one request from the head of the swap queue and process it
  // it may be read or write action
  // if pready is sendtoReady, then put the process back to ready queue
}

void *process_swapQ ()
{
  // called as the entry function for the swap thread
}

void start_swap_manager ()
{ 
  // initialize semaphores
  // initialize_swap_space ();
  // create swap thread
}

void end_swap_manager ()
{ 
  // terminate the swap thread 
}


