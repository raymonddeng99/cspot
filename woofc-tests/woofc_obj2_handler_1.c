#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "woofc.h"
#include "woofc-obj2.h"

int woofc_obj2_handler_1(WOOF *wf, unsigned long seq_no, void *ptr)
{

	OBJ2_EL *el = (OBJ2_EL *)ptr;

	if(el->counter < 10) {
		printf("name: %s, counter: %lu seq_no: %lu\n",wf->filename, el->counter, seq_no);
		fflush(stdout);
		el->counter++;
		printf("name: %s, calling WooFPut\n",wf->filename);
		fflush(stdout);
		WooFPut(wf->filename,"woofc_obj2_handler_1",(void *)el);
	}

	return(1);

}