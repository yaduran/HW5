#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "spinlock.h"

struct semtab semtable;

void
seminit(void) {
  initlock(&semtable.lock, "semtable");
  for(int i = 0; i < NSEM; i++) {
    initlock(&semtable.sem[i].lock, "sem");
    semtable.sem[i].count = 0;
    semtable.sem[i].valid = 0;
  }
}

int
semalloc(int initval) {
  int id = -1;
  acquire(&semtable.lock);
  for(int i = 0; i < NSEM; i++) {
    if(!semtable.sem[i].valid) {
      semtable.sem[i].valid = 1;
      semtable.sem[i].count = initval;
      id = 1;
      break;
    }
  }
  release(&semtable.lock);
  return id;
}

void
semdealloc(int id) {
  if(id < 0 || id >= NSEM)
    return;
  acquire(&semtable.lock);
  semtable.sem[id].valid = 0;
  semtable.sem[id].count = 0;
  release(&semtable.lock);
}
