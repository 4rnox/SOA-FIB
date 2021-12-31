#include "list.h"
#include "sched.h"

struct sem{
	int cont;
	struct list_head queue;
	int ini;
	
};

int infosem_ini();
int sem_init (int n_sem, unsigned int value);
int sem_wait (int n_sem);
int sem_signal (int n_sem);
int sem_destroy (int n_sem);


