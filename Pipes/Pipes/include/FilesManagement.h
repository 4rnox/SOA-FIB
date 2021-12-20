
#include "sem.h"


struct nodo{
	int posread;
	int poswrite;
	int bytesleft;
	int readers;
	int writers;
	struct sem lecsem;
	struct sem escsem;
	int free;
	int ini;
};

struct nodo tfo[20];

struct canal {
	int free;
	int lec;
	int entrada;
};

int add_entrada_lec(int);
int add_entrada_esc(int);


