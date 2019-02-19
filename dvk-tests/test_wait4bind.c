#include "tests.h"
   
void  main ( int argc, char *argv[] )
{
	int dcid, parent_pid, child_pid, parent_nr, child_nr; 
	int status, rcode, parent_ep, child_ep, ret;
	
    if ( argc != 4) {
		fprintf(stderr,  "Usage: %s <dcid> <parent_nr> <child_nr> \n", argv[0] );
 	    exit(EXIT_FAILURE);
	}

	dcid = atoi(argv[1]);
	if ( dcid < 0 || dcid >= NR_DCS) {
		fprintf(stderr,  "Invalid dcid [0-%d]\n", NR_DCS-1 );
		exit(EXIT_FAILURE);
	}

	parent_nr = atoi(argv[2]);
	parent_pid = getpid();
	child_nr = atoi(argv[3]);
	
	ret = dvk_open();
	if (ret < 0)  ERROR_PRINT(ret);
	
	if( fork() == 0 ){// CHILD ----------------------------------------------
		sleep(5);
		child_pid = getpid();
	    printf("CHILD: Binding process %d to DC%d with p_nr=%d\n",child_pid,dcid,child_nr);
		child_ep = dvk_bind(dcid, child_nr); 
		if( child_ep < EDVSERRCODE) ERROR_PRINT(child_ep);
		printf("CHILD: dvk_bind child_ep=%d\n", child_ep);

		sleep(5);
		printf("CHILD: exit\n");
		exit(0);
	} //------------------------------------------- END OF CHILD -------------------------------------

	printf("PARENT: Test for self binding BEFORE binding \n");
	rcode = dvk_wait4bind_T(1000);
	printf("PARENT: dvk_wait4bind_T SELF rcode=%d\n", rcode);
	
    printf("PARENT: binding itself (%d) to DC%d with parent_nr=%d\n",parent_pid,dcid,parent_nr);
	parent_ep = dvk_bind(dcid, parent_nr); 
	if( parent_ep < EDVSERRCODE) ERROR_PRINT(parent_ep);
	printf("PARENT: dvk_bind rcode=%d\n", parent_ep);

	printf("PARENT: Test for self binding AFTER binding \n");
	rcode = dvk_wait4bind_T(1000);
	printf("PARENT: dvk_wait4bind_T SELF rcode=%d\n", rcode);

	printf("PARENT: waiting for child binding: %d\n", child_nr);
	while(1) { 
		rcode = dvk_wait4bindep_T(child_nr, 1000);
		printf("PARENT: waiting for child binding rcode=%d\n", rcode);
		if(rcode == (-1)){
			fprintf(stderr, "PARENT: waiting for child binding TIMEOUT\n");
			continue;
		}else if (rcode == child_nr){
			break;				
		} else {
			ERROR_PRINT(rcode);	
		}
	} 
	printf("PARENT: Child %d is bound\n", rcode);

	printf("PARENT: waiting for child %d unbound\n", child_nr);
	while(1) { 
		rcode = dvk_wait4unbind_T(child_nr, 1000);
		printf("PARENT: waiting for child unbinding rcode=%d\n", rcode);
		if (rcode == 0) break;
		if(rcode == (-1)) {
			printf("PARENT: waiting for child unbinding TIMEOUT\n");
			continue;
		}else {
			ERROR_PRINT(rcode);		
		}
	} 
	printf("PARENT: dvk_wait4unbind_T rcode=%d\n", rcode);
#ifdef ANULADO		
#endif ANULADO	
	wait(&status);
 }



