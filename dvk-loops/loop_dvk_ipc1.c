#include "loops.h"


#define 	DCID	0
#define 	SRC_NR	1
#define 	DST_NR	2
#define		FORK_WAIT_MS	500

double dwalltime()
{
	double sec;
	struct timeval tv;

	gettimeofday(&tv,NULL);
	sec = tv.tv_sec + tv.tv_usec/1000000.0;
	return sec;
}

   
void  main ( int argc, char *argv[] )
{
	int dcid, src_pid, dst_pid, src_ep, dst_ep, src_nr, dst_nr, ret, i, loops;
	message *m_ptr;
	double t_start, t_stop, t_total;
	
  	if (argc != 2) {
    		printf ("usage: %s <loops>\n", argv[0]);
    		exit(1);
  	}

  	loops = atoi(argv[1]);

	dcid 	= DCID;
	src_nr	= SRC_NR;
	dst_nr 	= DST_NR;
	
	ret = dvk_open();
	if (ret < 0)  ERROR_EXIT(ret);

	if( (src_pid = fork()) != 0 )	{		/* PARENT = DESTINATION */

		/* BIND PARENT(DESTINATION) */
		dst_pid = getpid();
		dst_ep = dvk_bind(dcid, dst_nr);
		if( dst_ep < EDVSERRCODE) ERROR_EXIT(dst_ep);
		printf("BIND DESTINATION dcid=%d dst_pid=%d dst_nr=%d dst_ep=%d\n",
			dcid, dst_pid, dst_nr, dst_ep);
			
		/* BIND CHILD(SOURCE) */
    	src_ep =dvk_lclbind(dcid, src_pid, src_nr);
		if( src_ep < EDVSERRCODE) ERROR_EXIT(src_ep);
   		printf("BIND SOURCE dcid=%d src_pid=%d src_nr=%d src_ep=%d\n",
			dcid, src_pid, src_nr, 	src_ep);
			
		printf("RECEIVER pause before RECEIVE\n");
		sleep(1); /* PAUSE before RECEIVE*/

		posix_memalign( (void **) &m_ptr, getpagesize(), sizeof(message) );
		if (m_ptr== NULL) ERROR_EXIT(-errno);
		printf("m_ptr %p\n",m_ptr);
	
	   	ret = dvk_receive(ANY, (long) m_ptr); 
		if( ret < 0) ERROR_EXIT(ret);
		
		for( i = 0; i < (loops+10); i++) {
			if( i == 10) t_start = dwalltime();
		   	ret = dvk_receive(ANY, (long) m_ptr);		
			if( ret < 0) ERROR_EXIT(ret);
		   	ret = dvk_send(src_ep, (long) m_ptr);
			if( ret < 0) ERROR_EXIT(ret);
		}
     	t_stop  = dwalltime();

   		printf("RECEIVE %d: " MSG1_FORMAT, dst_pid, MSG1_FIELDS(m_ptr));	
		t_total = (t_stop-t_start);
 		printf("t_start=%.2f t_stop=%.2f t_total=%.2f\n",t_start, t_stop, t_total);
 		printf("Loops = %d\n", loops);
 		printf("Time for a pair of SEND/RECEIVE= %f[ms]\n", 1000*t_total/2/(double)loops);
 		printf("Throuhput = %f [SEND-RECEIVE/s]\n", (double)(loops*2)/t_total);
		wait(&ret);
		
	}else{						/* CHILD = SOURCE		*/

		printf("CHILD: dvk_wait4bind_T\n");
		do { 
			ret = dvk_wait4bind_T(FORK_WAIT_MS);
			printf("CHILD: dvk_wait4bind_T  ret=%d\n", ret);
			if (ret == EDVSTIMEDOUT) {
				printf("CHILD: dvk_wait4bind_T TIMEOUT\n");
				continue ;
			}else if( ret == EDVSNOTREADY){
				break;	
			}else if( ret < 0) 
				ERROR_EXIT(ret);
		} while	(ret < OK); 

		posix_memalign( (void **) &m_ptr, getpagesize(), sizeof(message) );
		if (m_ptr== NULL) ERROR_EXIT(-errno);
		printf("m_ptr %p\n",m_ptr);
		
		dst_pid = getppid();
		dst_ep = dvk_getep(dst_pid);
		if( dst_ep < EDVSERRCODE) ERROR_EXIT(dst_ep);

		src_pid = getpid();
	  	m_ptr->m_type= 0xFF;
		m_ptr->m1_i1 = 0x00;
		m_ptr->m1_i2 = 0x02;
		m_ptr->m1_i3 = 0x03;
   		printf("SEND %d: " MSG1_FORMAT, src_pid, MSG1_FIELDS(m_ptr));

		m_ptr->m1_i1 = 0x1234;
   		ret = dvk_send(dst_ep, (long) m_ptr);
		if( ret < 0) ERROR_EXIT(ret);

		for( i = 0; i < (loops+10); i++){
	    	ret = dvk_send(dst_ep, (long) m_ptr);
			if( ret < 0) ERROR_EXIT(ret);
	    	ret = dvk_receive(ANY, (long) m_ptr);
			if( ret < 0) ERROR_EXIT(ret);
		}

   		printf("RECEIVE %d: MSG1_FORMAT, ", src_pid, MSG1_FIELDS(m_ptr));
	}
	printf("exit \n");
	exit(0);
}



