#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "spinlock.h"
#include "proc.h" // Needed for sleep/wakeup related functions

struct semtab semtable; // [cite: 45]

// Initialize the semaphore table and locks.
void
seminit(void)
{
  initlock(&semtable.lock, "semtable"); // [cite: 47, 49]
  for (int i = 0; i < NSEM; i++) {
    initlock(&semtable.sem[i].lock, "sem"); // [cite: 50, 52]
  }
} // [cite: 54]

// Find an unused location in the semaphore table and mark it as valid.
// Returns the index (semid) or -1 (or 1 as per problem) if none available.
int
semalloc(void)
{
  // Acquire lock for the semaphore table before searching (concurrency control) [cite: 57]
  acquire(&semtable.lock);

  for (int i = 0; i < NSEM; i++) {
    if (semtable.sem[i].valid == 0) { // Found an unused entry [cite: 55]
      semtable.sem[i].valid = 1;
      release(&semtable.lock);
      return i; // Return the index of the allocated semaphore
    }
  }

  release(&semtable.lock);
  return -1; // No empty location. The prompt suggested 1, but -1 is standard for error/not-found.
} // [cite: 55]

// Invalidate an entry in the semaphore table.
void
semdealloc(int semid)
{
  // Acquire lock for the semaphore table
  acquire(&semtable.lock);

  // Check if semid is valid and in use before deallocating
  if (semid >= 0 && semid < NSEM && semtable.sem[semid].valid == 1) {
    // Mark the semaphore as invalid/free [cite: 56]
    semtable.sem[semid].valid = 0;
  }
  
  release(&semtable.lock);
}
