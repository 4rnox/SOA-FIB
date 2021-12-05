#include "list.h"
#include "sched.h"
int infosem_ini();
int sem_init (int n_sem, unsigned int value);
int sem_wait (int n_sem);
int sem_signal (int n_sem);
int sem_destroy (int n_sem);


