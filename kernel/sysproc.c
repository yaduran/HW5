#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;
  struct proc *p = myproc();

  if(argint(0, &n) < 0)
    return -1;

  addr = p->sz;
  if (n == 0)
    return addr;

  uint64 new_sz = addr + n;
  if(new_sz < p->sz){
    return (uint64)-1;
  }
  p->sz = new_sz;
  /*old eager allocatoin, we don't call growproc right away for lazy allocatoin*/
  /*if(growproc(n) < 0)
    return -1;*/
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_freepmem(void)
{
  uint64 pages = kfreepages_count();
  return pages * PGSIZE;
}

extern struct semtab semtable;

// int sem_init(sem_t *sem, int pshared, unsigned int value);
uint64
sys_sem_init(void)
{
  uint64 sem_addr;
  int pshared;
  int value;
  
  // Get arguments from user space
  if (argaddr(0, &sem_addr) < 0 || argint(1, &pshared) < 0 || argint(2, &value) < 0)
    return -1;

  // For simplicity, we ignore pshared (assumed 1 for process shared)
  if (value < 0)
    return -1; // POSIX compliance check

  // 1. Allocate a new semaphore entry in the kernel table
  int semid = semalloc();
  if (semid < 0) {
    return -1; // Allocation failed (table full)
  }

  // 2. Initialize the allocated semaphore's state
  struct semaphore *s = &semtable.sem[semid];
  acquire(&s->lock); // Lock the specific semaphore entry
  s->count = value;
  release(&s->lock);

  // 3. Copy the index (semid) back to the user's sem_t location [cite: 65]
  if (copyout(myproc()->pagetable, sem_addr, (char *)&semid, sizeof(semid)) < 0) {
    // If copyout fails, deallocate the semaphore
    semdealloc(semid);
    return -1;
  }

  return 0; // Success
}

// sys_sem_destroy(sem_t *sem)
uint64
sys_sem_destroy(void)
{
  uint64 sem_addr;
  int semid;

  // Get user address of sem_t
  if (argaddr(0, &sem_addr) < 0)
    return -1;
  
  // 1. Copy the semaphore index (semid) from user space [cite: 65]
  if (copyin(myproc()->pagetable, (char *)&semid, sem_addr, sizeof(semid)) < 0) {
    return -1;
  }

  // Check if semid is valid
  if (semid < 0 || semid >= NSEM || semtable.sem[semid].valid == 0) {
    return -1;
  }

  // 2. Wake up all processes potentially sleeping on this semaphore
  // This is required for POSIX compliance, though in xv6 you may simply deallocate.
  // wakeup(&semtable.sem[semid]); // Assuming a call to wakeup()

  // 3. Deallocate the semaphore entry in the kernel table
  semdealloc(semid);

  return 0; // Success
}

// sys_sem_wait(sem_t *sem) (P operation, decrement)
uint64
sys_sem_wait(void)
{
  uint64 sem_addr;
  int semid;

  // Get user address of sem_t
  if (argaddr(0, &sem_addr) < 0)
    return -1;

  // 1. Copy the semaphore index (semid) from user space [cite: 65]
  if (copyin(myproc()->pagetable, (char *)&semid, sem_addr, sizeof(semid)) < 0) {
    return -1;
  }

  // Check if semid is valid
  if (semid < 0 || semid >= NSEM || semtable.sem[semid].valid == 0) {
    return -1;
  }
  
  struct semaphore *s = &semtable.sem[semid];

  acquire(&s->lock);
  while (s->count <= 0) {
    // Wait/Sleep on the semaphore's address until count > 0 (wait until resource is available)
    // sleep(channel, lock) is the xv6 pattern for waiting.
    sleep(s, &s->lock); 
  }

  // Decrement the count (Acquire the resource)
  s->count--;
  release(&s->lock);

  return 0; // Success
}

// sys_sem_post(sem_t *sem) (V operation, increment)
uint64
sys_sem_post(void)
{
  uint64 sem_addr;
  int semid;

  // Get user address of sem_t
  if (argaddr(0, &sem_addr) < 0)
    return -1;

  // 1. Copy the semaphore index (semid) from user space [cite: 65]
  if (copyin(myproc()->pagetable, (char *)&semid, sem_addr, sizeof(semid)) < 0) {
    return -1;
  }

  // Check if semid is valid
  if (semid < 0 || semid >= NSEM || semtable.sem[semid].valid == 0) {
    return -1;
  }

  struct semaphore *s = &semtable.sem[semid];

  acquire(&s->lock);
  // Increment the count (Release the resource)
  s->count++;

  // Wake up a waiting process (if any)
  wakeup(s);

  release(&s->lock);
  
  return 0; // Success
}
