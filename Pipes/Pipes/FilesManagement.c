#include "FilesManagement.h"

extern struct sem;

int add_entrada_lec(int c){
	int canal = 0;
	for (int i = 3; i < 16 && i != -1 && current()->canals[i].lec = -1; ++i) if (current()->canals[i].free = 1) {canal = i; i = -1; }
	
	if (i == -1){
		current()->canals[canal].free = 0;
		current()->canals[canal].lec = 1;
		current()->canals[canal].entrada = c;
		return canal;	
	}

	else return -1;

}

int add_entrada_esc(int c){
	int canal = 0;
	for (int i = 3; i < 16 && i != -1 && current()->canals[i].lec = -1; ++i) if (current()->canals[i].free = 1) {canal = i; i = -1; }
	
	if (i == -1){
		current()->canals[canal].free = 0;
		current()->canals[canal].lec = 0;
		current()->canals[canal].entrada = c;
		return canal;	
	}

	else return -1;

}