include "sem.h"



extern struct list_head blocked;
struct sem infosem[5];
int infosem_ini(){
	for (int i = 0; i < 30; ++i) infosem[i].ini = 0;
	return 0;
}

int sem_init (int n_sem, unsigned int value){
	if (infosem[n_sem].ini == 1) return -1;
	struct sem aux;
	INIT_LIST_HEAD(&aux.queue);
	aux.ini = 1;
	aux.cont = 0;
	infosem[n_sem]= aux;
	return 0;
}

int sem_wait (int n_sem){
	if (infosem[n_sem].ini == 0 || current()->sembit == 1) {
	current()->sembit = 0;
	return -1;
	}
	if (infosem[n_sem].cont <= 0) {
	list_add_tail(&(current()->list), &infosem[n_sem].queue);
	sched_next_rr();
	}
	
	else --infosem[n_sem].cont;
	if (infosem[n_sem].ini == 0 || current()->sembit == 1) {
	current()->sembit = 0;
	return -1;
	}
	else return 0;
}

int sem_signal (int n_sem){
	if (infosem[n_sem].ini == 0) return -1;
	++infosem[n_sem].cont;
	
	if (!(list_empty(&infosem[n_sem].queue))){
	struct list_head *aux = list_first(&infosem[n_sem].queue);
	list_del(aux);
	update_process_state_rr(list_head_to_task_struct(aux), &readyqueue);
	
	}
	return 0;
}

int sem_destroy (int n_sem){
	if (infosem[n_sem].ini == 0) return -1;
	infosem[n_sem].ini = 0;
	while (!(list_empty(&infosem[n_sem].queue))){
		struct list_head *aux = list_first(&infosem[n_sem].queue);
		list_del(aux);
		list_head_to_task_struct(aux)->sembit = 1;
		list_add_tail(aux, &readyqueue);
	}
	update_process_state_rr(list_head_to_task_struct(aux), &readyqueue);
	return 0;
}