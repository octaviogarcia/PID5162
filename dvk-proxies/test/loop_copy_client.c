#include "loops.h"


#define 	RMTNODE	0
#define 	DCID	0
#define 	CLT_NR	1
#define 	SVR_NR	(CLT_NR - 1)
#define 	MAXCHILDREN	100

int dcid, children;
message *m_ptr;
char *buffer;

int child_function(int child) {
	int i , ret;
	pid_t child_pid;
	int child_nr, server_nr, child_ep;
	char rmt_name[16];
	
	child_nr = CLT_NR + ((child+1) * 2);
	printf("CHILD child_nr=%d: child=%d\n", child_nr, child);

	/*---------------- Allocate memory for message  ---------------*/
	posix_memalign( (void **) &m_ptr, getpagesize(), sizeof(message) );
	if (m_ptr== NULL) {
   		fprintf(stderr, "CHILD child_nr=%d: message posix_memalign\n", child_nr);
   		exit(1);
	}
	printf("CHILD child_nr=%d: m_ptr=%p\n",child_nr, m_ptr);	
	
	/*---------------- Allocate memory for DATA BUFFER ---------------*/
	posix_memalign( (void**) &buffer, getpagesize(), MAXCOPYLEN);
	if (buffer== NULL) {
   		fprintf(stderr, "CHILD child_nr=%d: buffer posix_memalign\n", child_nr);
   		exit(1);
  	}
	
	/*---------------- Fill with EDVSacters the DATA BUFFER ---------------*/
	for(i = 0; i < MAXCOPYLEN-1; i++)
		buffer[i] = ((i%25) + 'A');	
	buffer[MAXCOPYLEN] = 0;
	
	buffer[60] = 0;	
	USRDEBUG("CHILD child_nr=%d: buffer before = %s\n", child_nr, buffer);

	/* binding remote server */
	server_nr = child_nr-1;
	sprintf(rmt_name,"server%d",server_nr);
	ret = dvk_rmtbind(dcid, rmt_name, server_nr, RMTNODE);
	if(ret < 0) {
		fprintf(stderr,"CHILD child_nr=%d: dvk_rmtbind %d process %s on node %d \n", 
			child_nr, server_nr , rmt_name, RMTNODE);
		exit(1);		
	}
	
	/* binding local client */
  	child_nr = CLT_NR + ((child+1) * 2);
	child_ep = dvk_bind(dcid, child_nr);
	child_pid = getpid();
	USRDEBUG("CHILD child_nr=%d: child_ep=%d child_pid=%d\n", 
		child_nr, child_ep, child_pid);

	/* START synchronization with MAIN CLIENT */
	ret = dvk_sendrec(CLT_NR, (long) m_ptr);
	if(ret < 0) {
		fprintf(stderr,"CHILD child_nr=%d: dvk_sendrec ret=%d\n", child_nr, ret);
		exit(1);		
	}	
	
	/* M3-IPC TRANSFER LOOP  */
	m_ptr->m1_i1 = 1;
	m_ptr->m1_i2 = 2;
	m_ptr->m1_i3 = 3;
	m_ptr->m1_p1 = buffer;
 	USRDEBUG("CHILD child_nr=%d:Sending message to start loop. buffer=%X\n", child_nr, buffer);
	ret = dvk_sendrec(server_nr, (long) m_ptr);
	if(ret < 0) {
		fprintf(stderr,"CHILD child_nr=%d: dvk_sendrec ret=%d\n", child_nr, ret);
		exit(1);		
	}	
	buffer[30] = 0;	
	USRDEBUG("CHILD child_nr=%d: buffer after = %s\n", child_nr, buffer);
	
	/* STOP synchronization with MAIN CLIENT */
	ret = dvk_sendrec(CLT_NR, (long) m_ptr);
	if(ret < 0) {
		fprintf(stderr,"CHILD child_nr=%d: dvk_sendrec ret=%d\n", child_nr, ret);
		exit(1);		
	}	
	
	USRDEBUG("CHILD child_nr=%d: unbinding %d\n",child_nr, server_nr);
	dvk_unbind(dcid,server_nr);
	USRDEBUG("CHILD child_nr=%d:exiting\n", child_nr);
	exit(0);
}

double dwalltime()
{
	double sec;
	struct timeval tv;

	gettimeofday(&tv,NULL);
	sec = tv.tv_sec + tv.tv_usec/1000000.0;
	return sec;
}
   
int  main ( int argc, char *argv[] )
{
	int  clt_pid, ret, i, pid, clt_ep,child_ep, child_nr;
	double t_start, t_stop, t_total;
	pid_t child_pid[MAXCHILDREN];
	struct dc_usr dc, *dc_usr_ptr;
	char rmt_name[16];

  	if (argc != 2) {
    	fprintf(stderr,"usage: %s <children> \n", argv[0]);
    	exit(1);
  	}

	ret = dvk_open();
	if (ret < 0)  ERROR_EXIT(ret);
	
	
	/*---------------- Get DC info ---------------*/
	dcid 	= DCID;
	dc_usr_ptr = &dc;
	ret = dvk_getdcinfo(dcid, dc_usr_ptr);
	if(ret < 0) {
 	    fprintf(stderr,"dvk_getdcinfo error=%d \n", ret );
 	    exit(1);
	}
	USRDEBUG(DC_USR1_FORMAT,DC_USR1_FIELDS(dc_usr_ptr));
	USRDEBUG(DC_USR2_FORMAT,DC_USR2_FIELDS(dc_usr_ptr));
	
	/*---------------- Check Arguments ---------------*/
	children = atoi(argv[1]);

	if( children < 0 || children > (dc_usr_ptr->dc_nr_sysprocs - dc_usr_ptr->dc_nr_tasks) ){
   		fprintf(stderr, "children must be > 0 and < %d\n", 
			(dc_usr_ptr->dc_nr_sysprocs - dc_usr_ptr->dc_nr_tasks));
   		exit(1);
  	}
	
	/*---------------- MAIN CLIENT bind --------------*/
	clt_pid = getpid();
    clt_ep =	dvk_bind(dcid, CLT_NR);
	if( clt_ep < 0 ) {
		fprintf(stderr, "BIND ERROR clt_ep=%d\n",clt_ep);
	}
   	USRDEBUG("BIND MAIN CLIENT dcid=%d clt_pid=%d CLT_NR=%d clt_ep=%d\n",
		dcid, clt_pid, CLT_NR, 	clt_ep);

	/*--------------- binding remote server ---------*/
	sprintf(rmt_name,"server%d",SVR_NR);
	ret = dvk_rmtbind(dcid, rmt_name, SVR_NR, RMTNODE);
	if(ret < 0) {
    	fprintf(stderr,"ERROR MAIN CLIENT dvk_rmtbind %d: process %s on node %d \n", 
			SVR_NR, rmt_name, RMTNODE);
    	exit(1);		
	}
   	USRDEBUG("MAIN CLIENT dvk_rmtbind %d: process %s on node %d \n", 
			SVR_NR, rmt_name, RMTNODE);
		
	/*---------------- Creat children ---------------*/
	for( i = 0; i < children; i++){	
		USRDEBUG("child fork %d\n",i);
		if( (child_pid[i] = fork()) == 0 ){/* Child */
			ret = child_function( i );
		}
		/* parent */
		USRDEBUG("MAIN CLIENT child_pid[%d]=%d\n",i, child_pid[i]);
	}
			
	/*---------------- Allocate memory for message  ---------------*/
	posix_memalign( (void **) &m_ptr, getpagesize(), sizeof(message) );
	if (m_ptr== NULL) {
   		fprintf(stderr, "MAIN CLIENT: posix_memalign\n");
   		exit(1);
	}
	printf("MAIN CLIENT: m_ptr=%p\n", m_ptr);			
			
	/*--------- Waiting for children START synchronization: REQUEST ---------*/
	for( i = 0; i < children; i++){
		child_nr = CLT_NR + ((i+1) * 2);
		do {
			ret = dvk_receive(child_nr, (long) m_ptr);
			if (ret == EDVSSRCDIED)sleep(1);
		} while (ret == EDVSSRCDIED);
		if(ret < 0) {
			fprintf(stderr,"ERROR MAIN CLIENT: dvk_receive ret=%d\n", ret);
			exit(1);		
		}		
	}		
	
	/*--- Sending START message to remote SERVER ----*/
 	USRDEBUG("MAIN CLIENT: Sending START message to remote SERVER\n");
	ret = dvk_sendrec(SVR_NR, (long) m_ptr);
	if(ret < 0) {
		fprintf(stderr,"ERROR MAIN CLIENT: dvk_sendrec ret=%d\n", ret);
		exit(1);		
	}
	
	/*--------- Sending children START synchronization: REPLY ---------*/
	for( i = 0; i < children; i++){
		child_nr = CLT_NR + ((i+1) * 2);
   		ret = dvk_send(child_nr, (long) m_ptr);
		if(ret < 0) {
			fprintf(stderr,"ERROR MAIN CLIENT: dvk_send ret=%d\n", ret);
			exit(1);		
		}			
	}	
	
	/*--------- Waiting for children  STOP synchronization ---------*/
	for( i = 0; i < children; i++){
		child_nr = CLT_NR + ((i+1) * 2);
    	ret = dvk_receive(child_nr, (long) m_ptr);
		if(ret < 0) {
			fprintf(stderr,"ERROR MAIN CLIENT: dvk_receive ret=%d\n", ret);
			exit(1);		
		}		
	}	

	/*--- Sending STOP message to remote SERVER ----*/
 	USRDEBUG("MAIN CLIENT: Sending STOP message to remote SERVER\n");
	ret = dvk_sendrec(SVR_NR, (long) m_ptr);

	/*--------- Sending replies to children --------*/
	for( i = 0; i < children; i++){
		child_nr = CLT_NR + ((i+1) * 2);
   		ret = dvk_send(child_nr, (long) m_ptr);
		if(ret < 0) {
			fprintf(stderr,"ERROR MAIN CLIENT: dvk_send ret=%d\n", ret);
			exit(1);		
		}			
	}	

	sleep(3);
	
	/*--------- Unbinding remote MAIN SERVER ----*/
	dvk_unbind(dcid,SVR_NR);

	/*--------- Waiting for children EXIT ---------*/
	for( i = 0; i < children; i++){	
		wait(&ret);
	}
	
	printf("MAIN CLIENT END\n");

}



