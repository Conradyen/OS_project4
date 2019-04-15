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
  /**
   * Ming-Hsuan
   * @param maxProcess [description]
   */
  // reference the previous code for this part
  // but previous code was not fully completed
  int location, ret, retsize, k;

    if (pid < 2 || pid > maxProcess)
    { printf ("Error: Incorrect pid for disk read: %d\n", pid);
      return (-1);
    }
    location = (pid-2) * PswapSize + page*pagedataSize;
    ret = lseek (diskfd, location, SEEK_SET);
    if (ret < 0) perror ("Error lseek in read: \n");
    retsize = read (diskfd, (char *)buf, pagedataSize);
    if (retsize != pagedataSize)
    { printf ("Error: Disk read returned incorrect size: %d\n", retsize);
      exit(-1);
    }
    usleep (diskRWtime);
}

int write_swap_page (int pid, int page, unsigned *buf)
{
  /**
   * Ming-Hsuan
   * @param maxProcess [description]
   */
  // reference the previous code for this part
  // but previous code was not fully completed

  int location, ret, retsize;

    if (pid < 2 || pid > maxProcess)
    { printf ("Error: Incorrect pid for disk write: %d\n", pid);
      return (-1);
    }
    location = (pid-2) * PswapSize + page*pagedataSize;
    ret = lseek (diskfd, location, SEEK_SET);
    if (ret < 0) perror ("Error lseek in write: \n");
    retsize = write (diskfd, (char *)buf, pagedataSize);
    if (retsize != pagedataSize)
      { printf ("Error: Disk read returned incorrect size: %d\n", retsize);
        exit(-1);
      }
    usleep (diskRWtime);
}

int dump_process_swap_page (int pid, int page)
{
  /**
   * Ming-Hsuan
   * @param maxProcess [description]
   */
  // reference the previous code for this part
  // but previous code was not fully completed

  int location, ret, retsize, k;
    int buf[pageSize];

    if (pid < 2 || pid > maxProcess)
    { printf ("Error: Incorrect pid for disk dump: %d\n", pid);
      return (-1);
    }
    location = (pid-2) * PswapSize + page*pagedataSize;
    ret = lseek (diskfd, location, SEEK_SET);
    //printf ("loc %d %d %d, size %d\n", pid, page, location, pagedataSize);
    if (ret < 0) perror ("Error lseek in dump: \n");
    retsize = read (diskfd, (char *)buf, pagedataSize);
    if (retsize != pagedataSize)
    { printf ("Error: Disk dump read incorrect size: %d\n", retsize);
      exit(-1);
    }
    printf ("Content of process %d page %d:\n", pid, page);
    for (k=0; k<pageSize; k++) printf ("%d ", buf[k]);
    printf ("\n");
}

void dump_process_swap (int pid)
{ int j;
  printf ("****** Dump swap pages for process %d\n", pid);
  for (j=0; j<maxPpages; j++) dump_process_swap_page (pid, j);
}

// open the file with the swap space size, initialize content to 0
void initialize_swap_space ()
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
  /**
   * Ming-Hsuan
   * @param [name] [description]
   */
  // dump all the nodes in the swapQ
  SwapQnode *node = swapQhead;
  printf ("******************** Swap Queue Dump **************************\n");
  while(node != NULL){
    print_one_swapnode(node);
    node = node->next;
  }
}

// act can be actRead or actWrite
// pready indicates whether to put the process to ready queue after swap
// pready can be sendtoReady (back to ready queue) or notReady
void insert_swapQ (pid, page, buf, act, pready)
int pid, page, act, pready;
unsigned *buf;
{
  /**
   * Ming-Hsuan
   * @param SwapQnode [description]
   */
  SwapQnode *node = (SwapQnode *) malloc(sizeof(SwapQnode));
  node->pid = pid;
  node->page = page;
  node->act = act;
  node->pready = pready;
  //buffer

  if(swapQtail != NULL){
    swapQtail = node; swapQhead = node;
  }else{
    swapQtail->next = node; swapQtail = node;
  }

}

void process_one_swap ()
{ SwapQnode *node;
  /**
   * Ming-Hsuan
   * @param act [description]
   */
  // get one request from the head of the swap queue and process it
  // it may be read or write action
  // if pready is sendtoReady, then put the process back to ready queue
  if(Debug){
    dump_swapQ ();
  }
  if(swapQhead == NULL){
    printf("No process in swqp queue !!! \n");
  }
  else{
  node = swapQhead;
  // typedef struct SwapQnodeStruct
  // { int pid, page, act, pready;
  //   unsigned *buf;
  //   struct SwapQnodeStruct *next;
  // } SwapQnode;
    if(node->act = actRead){
      read_swap_page (node->pid, node->page, node->buf);
    }else if(node->act = actWrite){
      //if avtWrite write to swap.disk
      write_swap_page (node->pid, node->page, node->buf);
    }
    if(node->pready = sendtoReady){
      insert_ready_process(node->pid);
    }
  }
  if(Debug){
    printf("Remove swap queue pid : %d, Page: %d",node->pid,node->page);
  }
  swapQhead = node->next;
  if(swapQhead == NULL){
    swapQtail = NULL;
  }
  free(node);
  if(Debug) {
    dump_swapQ ();
  }

}

void *process_swapQ ()
{
  // called as the entry function for the swap thread
  while(systemActive){
    process_one_swap ();
    if(Debug){
      printf("swap queue has ended \n");
    }
  }
}

pthread_t swap_thread;

void start_swap_manager ()
{int ret;
  /**
   * Ming-Hsuan
   * @param swap_semaq [description]
   */

  sem_init(&swap_semaq,0,1);
  sem_init(&swapq_mutex,0,1);
  sem_init(&disk_mutex,0,1);
  // initialize semaphores
  initialize_swap_space ();// initialize_swap_space ();
  // create swap thread
  ret = pthread_create(&swap_thread,NULL,process_swapQ,NULL);
  if(ret < 0){
    printf("swap thread creation problem\n");
  }else{
    printf("swap thread has been created successfully\n");
  }

}

void end_swap_manager ()
{ int ret;
  // terminate the swap thread
  ret = pthread_join(swap_thread,NULL);
  printf("swap thread has terminated %d\n", ret);
}
