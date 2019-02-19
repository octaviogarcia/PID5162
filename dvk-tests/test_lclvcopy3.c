#include "tests.h"
   
char *buffer;
   
void  main ( int argc, char *argv[] )
{
	int parent_pid, parent_ep, parent_nr;
	int child1_pid, child1_ep, child1_nr;
	int child2_pid, child2_ep, child2_nr;
	int i, dcid,  ret, maxbuf;
	message *m_ptr, *m1_ptr, *m2_ptr;

	m1_ptr = (message *) malloc(sizeof(message));
	memset((char *) m1_ptr,0x00,sizeof(message));

	m2_ptr = (message *) malloc(sizeof(message));
	memset((char *) m2_ptr,0x00,sizeof(message));
	
	m_ptr = (message *) malloc(sizeof(message));
	memset((char *) m_ptr,0x00,sizeof(message));
	
    if ( argc != 6) {
		fprintf(stderr,  "Usage: %s <dcid> <parent_nr> <child1_nr> <child2_nr> <maxbuf>  \n", argv[0] );
		exit(EXIT_FAILURE);
	}

	dcid = atoi(argv[1]);
	if ( dcid < 0 || dcid >= NR_DCS) {
		fprintf(stderr,  "Invalid dcid [0-%d]\n", NR_DCS-1 );
		exit(EXIT_FAILURE);
	}

	parent_ep = parent_nr = atoi(argv[2]);
	child1_ep = child1_nr = atoi(argv[3]);
	child2_ep = child2_nr = atoi(argv[4]);
	
	maxbuf = atoi(argv[5]);
	if( maxbuf <= 0 || maxbuf > MAXCOPYLEN ) {
		fprintf(stderr, "Bad buffer size =%d\n", maxbuf);
		exit(EXIT_FAILURE);
	}
		
	ret = dvk_open();
	if (ret < 0)  ERROR_PRINT(ret);
	parent_pid = getpid();
	
	if( child1_pid = fork() ) { //  PARENT
	
		if( child2_pid = fork() ) { //  PARENT
			
			parent_ep =	dvk_bind(dcid, parent_nr);
			if( parent_ep < EDVSERRCODE) ERROR_PRINT(parent_ep);	
			printf("PARENT BIND dcid=%d parent_pid=%d parent_nr=%d parent_ep=%d m_ptr=%p\n",
					dcid,
					parent_pid,
					parent_nr,
					parent_ep,
					m_ptr);
			
			sleep(3); 
			
			printf("CHILD1 RECEIVE\n");
			ret = dvk_receive(child1_ep, (long) m1_ptr);
			if( ret != 0 )  ERROR_PRINT(ret);
			printf("CHILD1 RECEIVE msg:" MSG1_FORMAT, MSG1_FIELDS(m1_ptr));	
			
			printf("CHILD2 RECEIVE\n");
			ret = dvk_receive(child2_ep, (long) m2_ptr);
			if( ret != 0 )  ERROR_PRINT(ret);
			printf("CHILD2 RECEIVE msg:" MSG1_FORMAT, MSG1_FIELDS(m2_ptr));
		
			printf("CHILD1 to CHILD2 VCOPY child1_ep=%d to child2_ep=%d\n", child1_ep, child2_ep);
			ret = dvk_vcopy( child1_ep, m1_ptr->m1_p1, child2_ep, m2_ptr->m1_p1, maxbuf);	
			if( ret != 0 ) ERROR_PRINT(ret);

			printf("CHILD1 SEND\n");
			m_ptr->m_type = 0xFE;
			ret = dvk_send(m1_ptr->m_source,(long) m_ptr);
			if( ret != 0 )  ERROR_PRINT(ret);
	
			printf("CHILD2 SEND\n");
			m_ptr->m_type = 0xFE;
			ret = dvk_send(m2_ptr->m_source,(long) m_ptr);
			if( ret != 0 )  ERROR_PRINT(ret);

			wait(0);
			wait(0);
			
		} else { // CHILD2
			/*---------------- Allocate memory for DATA BUFFER ---------------*/
			posix_memalign( (void**) &buffer, getpagesize(), maxbuf);
			if (buffer== NULL) {
				fprintf(stderr, "CHILD2 buffer posix_memalign\n");
				exit(EXIT_FAILURE);
			}

			child2_pid = getpid();
			child2_ep =	dvk_bind(dcid, child2_nr);
			if( child2_ep < EDVSERRCODE) ERROR_PRINT(child2_ep);
			printf("CHILD2 BIND dcid=%d child2_pid=%d child2_nr=%d child2_ep=%d buffer=%p\n",
				dcid, child2_pid,child2_nr,child2_ep,buffer);

			for ( i = 0 ; i < (maxbuf); i++) 
				buffer[i] = '0' + (i%10);
			if( maxbuf > 60) buffer[60] = 0;	
			printf("CHILD2 buffer before = %s\n", buffer);
		
			m_ptr->m_type= 0x0A;
			m_ptr->m1_i1 = maxbuf;
			m_ptr->m1_i2 = 0x02;
			m_ptr->m1_i3 = 0x03;
			m_ptr->m1_p1 = buffer;
			
			printf("CHILD2 SENDREC msg:" MSG1_FORMAT, MSG1_FIELDS(m_ptr));
			ret = dvk_sendrec(parent_ep, (long) m_ptr);
			if( ret != 0 ) ERROR_PRINT(ret);
			printf("CHILD2 REPLY msg:" MSG1_FORMAT, MSG1_FIELDS(m_ptr));
			printf("CHILD2 buffer after = %s\n", buffer);
		}
	}else{ // CHILD1
		/*---------------- Allocate memory for DATA BUFFER ---------------*/
		posix_memalign( (void**) &buffer, getpagesize(), maxbuf);
		if (buffer== NULL) {
			fprintf(stderr, "CHILD buffer posix_memalign\n");
			exit(EXIT_FAILURE);
		}
	
		child1_pid = getpid();
		child1_ep =	dvk_bind(dcid, child1_nr);
		if( child1_ep < EDVSERRCODE) ERROR_PRINT(child1_ep);
		printf("CHILD1 BIND dcid=%d child1_pid=%d child1_nr=%d child1_ep=%d buffer=%p\n",
				dcid, child1_pid,child1_nr,child1_ep,buffer);
				
		for ( i = 0 ; i < (maxbuf); i++) 
			buffer[i] = 'A' + (i%10);
		if( maxbuf > 60) buffer[60] = 0;	
		printf("CHILD1 buffer before = %s\n", buffer);
		
		m_ptr->m_type= 0x0A;
		m_ptr->m1_i1 = maxbuf;
		m_ptr->m1_i2 = 0x02;
		m_ptr->m1_i3 = 0x03;
		m_ptr->m1_p1 = buffer;		
		
		printf("CHILD1 SENDREC msg:" MSG1_FORMAT, MSG1_FIELDS(m_ptr));
		ret = dvk_sendrec(parent_ep, (long) m_ptr);
		if( ret != 0 ) ERROR_PRINT(ret);
		printf("CHILD1 REPLY msg:" MSG1_FORMAT, MSG1_FIELDS(m_ptr));
		printf("CHILD1 buffer after = %s\n", buffer);
	}
	
 }



