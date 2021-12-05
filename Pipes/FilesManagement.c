#include "FilesManagement.h"
extern struct sem;

struct nodo{
	int posread = 0;
	int poswrite = 0;
	int bytesleft = 0;
	int readers = 0;
	int writers = 0;
	sem lecsem;
	sem escsem;
	int free = 1;
};
struct nodo tfo[20];

struct canal {
	int free = 1;
	int lec = -1;
	int entrada = -1;
}

int add_entrada_lec(int c){
	int canal = 0;
	for (int i = 3; i < current()->canals.size() && i != -1 && current()->canals[i].lec = -1; ++i) if (current()->canals[i].free = 1) canal = i; i = -1; 
	
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
	for (int i = 3; i < current()->canals.size() && i != -1 && current()->canals[i].lec = -1; ++i) if (current()->canals[i].free = 1) canal = i; i = -1; 
	
	if (i == -1){
		current()->canals[canal].free = 0;
		current()->canals[canal].lec = 0;
		current()->canals[canal].entrada = c;
		return canal;	
	}

	else return -1;

}