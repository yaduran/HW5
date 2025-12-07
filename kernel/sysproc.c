#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

extern struct semtab semtable;

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

uint64
sys_sem_init(void) {

  uint64 addr;
  int pshared;
  int val;

  if(argaddr(0, &addr) < 0 || argaddr(1, &pshared) < 0 || argaddr(2, &val) < 0)
    return -1;

  int id = semalloc(val);
  if(id < 0)
    return -1;

  sem_t tempid = id;
  struct proc *p = myproc();
  if(copyout(p->pagetable, addr, (char *)&tempid, sizeof(tempid)) < 0) {
    semdealloc(id);
    return -1;
  }

  return 0;
}

uint64
sys_sem_destroy(void) {
  sem_t id;
  uint64 addr;

  if(argaddr(0, &addr) < 0)
    return -1
  struct proc *p = myproc()

  if(copyin(p-pagetable, (char *)&id, addr, sizeof(id)) < 0)
    return -1;

  if(id < 0 || id >= NSEM || !semtable.sem[id].valid)
    return -1
  semdealloc(id);
  return 0;
}

uint64
sys_sem_wait(void) {
  uint64 addr;
  sem_t id;

  if(argaddr(0, &addr) < 0)
    return -1;

  struct proc *p = myproc();
  if(copyin(p->pagetable, (char *)&id, addr, sizeof(id)) < 0)
      return -1;

  if(id < 0 || id >= NSEM || !semtable.sem[id].valid)
    return -1
  struct semaphore *s = &semtable.sem[id];

  acquire(&s->lock);
  while(s->count == 0) {
    sleep(s, &s->lock);
  }
  s->count--;
  release(&s->lock);
  return 0;
}

uint64
sys_sem_post(void) {
  sem_t id;
  uint64 addr;

  if(argaddr(0, &addr) < 0)
    return -1;
  struct proc *p = myproc();

  if(copyin(p->pagetable, (char *)&id, addr, sizeof(id)) < 0)
    return -1;

  if(id < 0 || id >= NSEM || !semtable.sem[id].valid)
    return -1;
  struct semaphore *s = &semtable.sem[id];
  acquire(&s->lock);
  s->count++;
  wakeup(s);
  release(&s->lock);
  return 0;
}
