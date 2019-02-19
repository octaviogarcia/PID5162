#include "tests.h"
   
   
void  main ( int argc, char *argv[] )
{
	int dcid, parent_pid, child_pid, parent_ep, child_ep, parent_nr, child_nr, ret;
	message *m_ptr;
	proc_usr_t usr_info, *p_usr;
	
	p_usr = &usr_info;
	m_ptr = (char *) malloc(sizeof(message));
	memset(m_ptr,0x00,sizeof(message));
	
    if ( argc != 4) {
		fprintf(stderr,  "Usage: %s <dcid> <parent_nr> <child_nr>  \n", argv[0] );
		exit(EXIT_FAILURE);
	}

	dcid = atoi(argv[1]);
	if ( dcid < 0 || dcid >= NR_DCS) {
		fprintf(stderr,  "Invalid dcid [0-%d]\n", NR_DCS-1 );
		exit(EXIT_FAILURE);
	}

	parent_nr = atoi(argv[2]);
	child_ep = child_nr = atoi(argv[3]);
	
	ret = dvk_open();
	if (ret < 0)  ERROR_PRINT(ret);
	
	parent_pid = getpid();
	
	if( child_pid = fork() ) { //  PARENT
		parent_ep =	dvk_bind(dcid, parent_nr);
		if( parent_ep < EDVSERRCODE) ERROR_PRINT(parent_ep);
		printf("PARENT BIND dcid=%d parent_pid=%d parent_nr=%d parent_ep=%d m_ptr=%p\n",
				dcid,
				parent_pid,
				parent_nr,
				parent_ep,
				m_ptr);
			

		printf("PARENT pause before SENDREC\n");
		// WAIT CHILD 
		sleep(3); 
		m_ptr->m_type= 0x0A;
		m_ptr->m1_i1 = 0x01;
		m_ptr->m1_i2 = 0x02;
		m_ptr->m1_i3 = 0x03;
				
		// FIRST SENDREC MESSAGE
		printf("PARENT FIRST SENDREC msg:" MSG1_FORMAT, MSG1_FIELDS(m_ptr));
		ret = dvk_sendrec(child_ep, (long) m_ptr);
		if( ret != 0 ) ERROR_PRINT(ret);
		printf("PARENT FIRST REPLY msg:" MSG1_FORMAT, MSG1_FIELDS(m_ptr));

		// SECOND SENDREC MESSAGE 
		m_ptr->m_type= 0x0B;
		m_ptr->m1_i1 = 0x05;
		m_ptr->m1_i2 = 0x06;
		m_ptr->m1_i3 = 0x07;
		printf("PARENT SECOND SENDREC msg:" MSG1_FORMAT, MSG1_FIELDS(m_ptr));
		ret = dvk_sendrec(child_ep, (long) m_ptr);
		if( ret != 0 ) ERROR_PRINT(ret);
		printf("PARENT SECOND REPLY msg:" MSG1_FORMAT, MSG1_FIELDS(m_ptr));

		
		wait(0);
		
		ret = dvk_getprocinfo(dcid, parent_nr, p_usr);
		if( ret < 0) ERROR_EXIT(ret);
		printf(PROC_USR_FORMAT, PROC_USR_FIELDS(p_usr));
		printf(PROC_WAIT_FORMAT, PROC_WAIT_FIELDS(p_usr));
		printf(PROC_COUNT_FORMAT, PROC_COUNT_FIELDS(p_usr));

	}else{ // CHILD 
		child_pid = getpid();
		child_ep =	dvk_bind(dcid, child_nr);
		if( child_ep < EDVSERRCODE) ERROR_PRINT(child_ep);
		printf("CHILD BIND dcid=%d child_pid=%d child_nr=%d child_ep=%d m_ptr=%p\n",
				dcid, child_pid,child_nr,child_ep,m_ptr);
				
		printf("CHILD FIRST RECEIVE\n");
		ret = dvk_receive(ANY, (long) m_ptr);
		if( ret != 0 ) ERROR_PRINT(ret);
		printf("CHILD RECEIVE msg:" MSG1_FORMAT, MSG1_FIELDS(m_ptr));	

		printf("CHILD FIRST SEND\n");
		m_ptr->m_type = 0xFE;
		ret = dvk_send(m_ptr->m_source,(long) m_ptr);
		if( ret != 0 ) ERROR_PRINT(ret);

		sleep(10);
		
		printf("CHILD SECOND RECEIVE\n");
		ret = dvk_receive(ANY, (long) m_ptr);
		if( ret != 0 ) 	ERROR_PRINT(ret);
		printf("CHILD RECEIVE msg:" MSG1_FORMAT, MSG1_FIELDS(m_ptr));	

		printf("CHILD SECOND SEND\n");
		m_ptr->m_type = 0xEF;
		ret = dvk_send(m_ptr->m_source,(long) m_ptr);
		if( ret != 0 ) ERROR_PRINT(ret);
	}
}




