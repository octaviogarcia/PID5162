#include "tests.h"
   
void  main ( int argc, char *argv[] )
{
	int dcid, parent_pid, child_pid, parent_ep, child_ep, parent_nr, child_nr, ret;
	message *m_ptr;

	
	m_ptr = (char *) malloc(sizeof(message));
	memset(m_ptr,0x00,sizeof(message));
	
    if ( argc != 3) {
		fprintf(stderr, "Usage: %s <dcid> <proc_nr>\n", argv[0] );
		exit(EXIT_FAILURE);
	}

	dcid = atoi(argv[1]);
	if ( dcid < 0 || dcid >= NR_DCS) {
 	    fprintf(stderr,  "Invalid dcid [0-%d]\n", NR_DCS-1 );
 	    exit(EXIT_FAILURE);
	}

	child_ep = child_nr = atoi(argv[2]);
	
	ret = dvk_open();
	if (ret < 0)  ERROR_PRINT(ret);
	
	parent_pid = getpid();
	
	if( child_pid = fork() ) { //  PARENT
		printf("PARENT pause before WAKEUP\n");
		// WAIT CHILD 
		sleep(3); 
				
		// FIRST SENDREC MESSAGE
		printf("PARENT WAKEUP child_ep=%d\n", child_ep);
		ret = dvk_wakeup(dcid, child_ep);
		if( ret != 0 ) ERROR_PRINT(ret);
		printf("PARENT wait for child\n");
		wait(0);
	}else{ // CHILD 
		child_pid = getpid();
		child_ep =	dvk_bind(dcid, child_nr);
		if( child_ep < EDVSERRCODE) ERROR_PRINT(child_ep);
		printf("CHILD BIND dcid=%d child_pid=%d child_nr=%d child_ep=%d m_ptr=%p\n",
				dcid, child_pid,child_nr,child_ep,m_ptr);
				
		printf("CHILD RECEIVE ANY\n");
		ret = dvk_receive(ANY, (long) m_ptr);
		if( ret != 0 ) ERROR_PRINT(ret);
		printf("CHILD RECEIVE msg:" MSG1_FORMAT, MSG1_FIELDS(m_ptr));	
	}
	
 }



