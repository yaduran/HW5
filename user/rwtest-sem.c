#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define NULL 0
#define READER_ITERS 50
#define WRITER_ITERS 100

typedef struct {
  int value;        // shared "database" value
  int readercount;  // number of active readers
  sem_t mutex;      // protects readercount
  sem_t wrt;        // writers' lock (and readers' first/last lock)
} rw_t;

rw_t *rw;

void
reader(void)
{
  // simple debug so you *know* a reader started
  // (you can delete this later)
  // printf("reader: pid %d starting\n", getpid());

  for (int i = 0; i < READER_ITERS; i++) {
    // ENTRY SECTION
    sem_wait(&rw->mutex);
    rw->readercount++;
    if (rw->readercount == 1) {
      // first reader blocks writers
      sem_wait(&rw->wrt);
    }
    sem_post(&rw->mutex);

    // CRITICAL SECTION: logically read rw->value
    // (we don't store it anywhere; only synchronization matters)
    // int v = rw->value;

    // EXIT SECTION
    sem_wait(&rw->mutex);
    rw->readercount--;
    if (rw->readercount == 0) {
      // last reader lets writers proceed
      sem_post(&rw->wrt);
    }
    sem_post(&rw->mutex);
  }

  exit(0);
}

void
writer(void)
{
  // debug start
  // printf("writer: pid %d starting\n", getpid());

  for (int i = 0; i < WRITER_ITERS; i++) {
    sem_wait(&rw->wrt);   // exclusive access
    rw->value++;          // update shared value
    sem_post(&rw->wrt);   // release
  }

  exit(0);
}

int
main(int argc, char *argv[])
{
  if (argc != 3) {
    printf("usage: %s <nreaders> <nwriters>\n", argv[0]);
    exit(0);
  }

  int nreaders = atoi(argv[1]);
  int nwriters = atoi(argv[2]);

  // quick debug so you know main actually runs
  // (if you never see this line, program never starts / crashes early)
  printf("rw-sem: starting with %d readers, %d writers\n",
         nreaders, nwriters);

  rw = (rw_t *) mmap(NULL, sizeof(rw_t),
                     PROT_READ | PROT_WRITE,
                     MAP_ANONYMOUS | MAP_SHARED, -1, 0);
  if (rw == (void *)-1 || rw == NULL) {
    printf("rw-sem: mmap failed\n");
    exit(1);
  }

  // initialize shared state
  rw->value = 0;
  rw->readercount = 0;
  sem_init(&rw->mutex, 1, 1);
  sem_init(&rw->wrt,   1, 1);

  // fork readers
  for (int i = 0; i < nreaders; i++) {
    int pid = fork();
    if (pid == 0) {
      reader();
    } else if (pid < 0) {
      printf("rw-sem: fork reader failed\n");
      exit(1);
    }
  }

  // fork writers
  for (int i = 0; i < nwriters; i++) {
    int pid = fork();
    if (pid == 0) {
      writer();
    } else if (pid < 0) {
      printf("rw-sem: fork writer failed\n");
      exit(1);
    }
  }

  // wait for all children
  for (int i = 0; i < nreaders + nwriters; i++)
    wait(0);

  // check final value
  int final = rw->value;
  int expected = nwriters * WRITER_ITERS;
  printf("rw-sem: final value = %d, expected = %d\n", final, expected);

  // cleanup
  sem_destroy(&rw->mutex);
  sem_destroy(&rw->wrt);
  munmap(rw, sizeof(rw_t));

  exit(0);
}
