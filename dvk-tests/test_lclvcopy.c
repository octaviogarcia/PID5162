#include "tests.h"
   
char *buffer;
   
void  main ( int argc, char *argv[] )
{
	int parent_pid, parent_ep, parent_nr;
	int child_pid, child_ep, child_nr;
	int i, dcid,  ret, maxbuf;
	message *m_ptr;

	m_ptr = (message *) malloc(sizeof(message));
	memset((char *) m_ptr,0x00,sizeof(message));
	
    if ( argc != 5) {
		fprintf(stderr,  "Usage: %s <dcid> <parent_nr> <child_nr> <maxbuf>  \n", argv[0] );
		exit(EXIT_FAILURE);
	}

	dcid = atoi(argv[1]);
	if ( dcid < 0 || dcid >= NR_DCS) {
		fprintf(stderr,  "Invalid dcid [0-%d]\n", NR_DCS-1 );
		exit(EXIT_FAILURE);
	}

	parent_nr = atoi(argv[2]);
	child_ep = child_nr = atoi(argv[3]);
	maxbuf = atoi(argv[4]);
	if( maxbuf <= 0 || maxbuf > MAXCOPYLEN ) {
		fprintf(stderr, "Bad buffer size =%d\n", maxbuf);
		exit(EXIT_FAILURE);
	}
		
	ret = dvk_open();
	if (ret < 0)  ERROR_PRINT(ret);
	parent_pid = getpid();
	
	if( child_pid = fork() ) { //  PARENT
	
		/*---------------- Allocate memory for DATA BUFFER ---------------*/
		posix_memalign( (void**) &buffer, getpagesize(), maxbuf);
		if (buffer== NULL) {
			fprintf(stderr, "PARENT buffer posix_memalign\n");
			exit(EXIT_FAILURE);
		}
		for ( i = 0 ; i < (maxbuf); i++) 
			buffer[i] = '0' + (i%10);
		if( maxbuf > 60) buffer[60] = 0;	
		printf("PARENT buffer before = %s\n", buffer);
	
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
		m_ptr->m1_i1 = maxbuf;
		m_ptr->m1_i2 = 0x02;
		m_ptr->m1_i3 = 0x03;
		m_ptr->m1_p1 = buffer;
				
		// FIRST SENDREC MESSAGE
		printf("PARENT FIRST SENDREC msg:" MSG1_FORMAT, MSG1_FIELDS(m_ptr));
		ret = dvk_sendrec(child_ep, (long) m_ptr);
		if( ret != 0 ) ERROR_PRINT(ret);
		printf("PARENT FIRST REPLY msg:" MSG1_FORMAT, MSG1_FIELDS(m_ptr));
		
		printf("PARENT buffer after = %s\n", buffer);

		// reset buffer content
		for ( i = 0 ; i < (maxbuf); i++) 
			buffer[i] = '0' + (i%10);
		if( maxbuf > 60) buffer[60] = 0;	
		printf("PARENT buffer before = %s\n", buffer);
		
		// SECOND SENDREC MESSAGE 
		m_ptr->m_type= 0x0B;
		m_ptr->m1_i1 = maxbuf;
		m_ptr->m1_i2 = 0x06;
		m_ptr->m1_i3 = 0x07;
		m_ptr->m1_p1 = buffer;
		printf("PARENT SECOND SENDREC msg:" MSG1_FORMAT, MSG1_FIELDS(m_ptr));
		ret = dvk_sendrec(child_ep, (long) m_ptr);
		if( ret != 0 )  ERROR_PRINT(ret);
		printf("PARENT SECOND REPLY msg:" MSG1_FORMAT, MSG1_FIELDS(m_ptr));
		
		wait(0);
	}else{ // CHILD 
		/*---------------- Allocate memory for DATA BUFFER ---------------*/
		posix_memalign( (void**) &buffer, getpagesize(), maxbuf);
		if (buffer== NULL) {
			fprintf(stderr, "CHILD buffer posix_memalign\n");
			exit(EXIT_FAILURE);
		}
		for ( i = 0 ; i < (maxbuf); i++) 
			buffer[i] = 'A' + (i%10);
		if( maxbuf > 60) buffer[60] = 0;	
		printf("CHILD buffer before = %s\n", buffer);
		
		child_pid = getpid();
		child_ep =	dvk_bind(dcid, child_nr);
		if( child_ep < EDVSERRCODE) ERROR_PRINT(child_ep);
		printf("CHILD BIND dcid=%d child_pid=%d child_nr=%d child_ep=%d m_ptr=%p\n",
				dcid, child_pid,child_nr,child_ep,m_ptr);
				
		printf("CHILD FIRST RECEIVE\n");
		ret = dvk_receive(ANY, (long) m_ptr);
		if( ret != 0 )  ERROR_PRINT(ret);
		printf("CHILD RECEIVE msg:" MSG1_FORMAT, MSG1_FIELDS(m_ptr));	

    	printf("CHILD VCOPY child_ep=%d to m_ptr->m_source=%d \n", child_ep, m_ptr->m_source);
		ret = dvk_vcopy( child_ep, buffer, m_ptr->m_source, m_ptr->m1_p1, maxbuf);	
		if( ret != 0 ) ERROR_PRINT(ret);

		printf("CHILD FIRST SEND\n");
		m_ptr->m_type = 0xFE;
		ret = dvk_send(m_ptr->m_source,(long) m_ptr);
		if( ret != 0 )  ERROR_PRINT(ret);
	
		printf("CHILD SECOND RECEIVE\n");
		ret = dvk_receive(ANY, (long) m_ptr);
		if( ret != 0 )  ERROR_PRINT(ret);
		printf("CHILD RECEIVE msg:" MSG1_FORMAT, MSG1_FIELDS(m_ptr));	

		printf("VCOPY m_ptr->m_source=%d to child_ep=%d \n", m_ptr->m_source, child_ep);
		ret = dvk_vcopy( m_ptr->m_source, m_ptr->m1_p1, child_ep, buffer, maxbuf);	
		if( ret != 0 )  ERROR_PRINT(ret);
		printf("CHILD buffer after = %s\n", buffer);
		
		printf("CHILD SECOND SEND\n");
		m_ptr->m_type = 0xEF;
		ret = dvk_send(m_ptr->m_source,(long) m_ptr);
		if( ret != 0 )  ERROR_PRINT(ret);
	}
	
 }



