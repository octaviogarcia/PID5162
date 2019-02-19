#include "loops.h"

#define 	DCID	0
#define 	SRC_NR	1
#define 	DST_NR	2


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
	int dcid, src_pid, dst_pid, src_ep, dst_ep, src_nr, dst_nr, ret, i, maxbuf;
	message m, *m_ptr;
	long int loops, total_bytes = 0 ;
	double t_start, t_stop, t_total,loopbysec, tput;
	char *buffer;
	
  	if (argc != 3) {
    		printf ("usage: %s <loops> <bufsize> \n", argv[0]);
    		exit(1);
  	}

  	loops = atoi(argv[1]);
  	maxbuf = atoi(argv[2]);

	posix_memalign( (void **) &m_ptr, getpagesize(), sizeof(message) );
	if (m_ptr== NULL) ERROR_EXIT(-errno);
	printf("m_ptr %p\n",m_ptr);

	posix_memalign( (void**) &buffer, getpagesize(), maxbuf);
	if (buffer== NULL) ERROR_EXIT(-errno);
	
	dcid 	= DCID;
	src_nr	= SRC_NR;
	dst_nr 	= DST_NR;
	
	ret = dvk_open();
	if (ret < 0)  ERROR_EXIT(ret);
	
	dst_pid = getpid();
    dst_ep =	dvk_bind(dcid, dst_nr);
	if( dst_ep < 0 ) 
		printf("BIND ERROR dst_ep=%d\n",dst_ep);

   	printf("BIND DESTINATION dcid=%d dst_pid=%d dst_nr=%d dst_ep=%d\n",
		dcid, dst_pid, dst_nr, dst_ep);


	if( (src_pid = fork()) != 0 )	{		/* PARENT */
		printf("RECEIVER pause before RECEIVE\n");
		sleep(1); /* PAUSE before RECEIVE*/
		do {
			src_ep = dvk_getep(src_pid);
			sleep(1);
		} while(src_ep < 0);
			
		for(i = 0; i < maxbuf; i++)
			buffer[i] = ((i%10) + '0');	
		if(maxbuf < 60) {
			printf("PARENT buffer:%s\n", buffer); 
			buffer[maxbuf] = 0;
		}
		
	    ret = dvk_receive(ANY, (long) &m);
		t_start = dwalltime();
		for( i = 0; i < loops; i++) {
			ret = dvk_vcopy(dst_ep, buffer, src_ep, buffer, maxbuf);	
		}
	   	t_stop  = dwalltime();
  		ret = dvk_send(src_ep, (long) &m);

		printf("UNBIND DESTINATION dcid=%d dst_pid=%d dst_nr=%d dst_ep=%d\n",
					dcid, dst_pid, 	dst_nr, dst_ep);
		dvk_unbind(dcid,dst_ep);

		t_total = (t_stop-t_start);
		total_bytes = loops*maxbuf;
		loopbysec = (double)(loops)/t_total;
		tput = loopbysec * (double)maxbuf;	
 		printf("t_start=%.2f t_stop=%.2f t_total=%.2f\n",t_start, t_stop, t_total);
 		printf("transfer size=%d #transfers=%d loopbysec=%f\n", maxbuf, loops, loopbysec);
 		printf("Throuhput = %f [bytes/s]\n", tput);
		wait(&ret);

	}else{						/* CHILD 		*/
		for(i = 0; i < maxbuf; i++)
			buffer[i] = ((i%10) + 'A');	
		if(maxbuf < 60) {
			printf("CHILD BEFORE buffer:%s\n", buffer); 
			buffer[maxbuf] = 0;
		}
		
		src_pid = getpid();
    	src_ep =dvk_bind(dcid, src_nr);
		if( src_ep < 0 ) 
			printf("BIND ERROR src_ep=%d\n",src_ep);

   		printf("BIND SOURCE dcid=%d src_pid=%d src_nr=%d src_ep=%d\n",
			dcid, src_pid, src_nr, src_ep);

	  	m_ptr->m_type= 0x0001;
		m_ptr->m1_i1 = 0x00;
		m_ptr->m1_i2 = 0x02;
		m_ptr->m1_i3 = 0x03;
		
   		printf("SEND msg: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));	
   		ret = dvk_sendrec(dst_ep, (long) &m);

		if(maxbuf < 60) {
			printf("CHILD AFTER buffer:%s\n", buffer); 
			buffer[maxbuf] = 0;
		}
		
   		printf("UNBIND SOURCE dcid=%d src_pid=%d src_nr=%d src_ep=%d\n",
			dcid, src_pid, src_nr, src_ep);
	    dvk_unbind(dcid, src_ep);
	}
 printf("\n");
 

 
 exit(0);
}



