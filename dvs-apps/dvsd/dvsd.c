/****************************************************************/
/* 				DVSD				*/
/*  The DVS Deamon which allow DVS cluster management 			*/
/*  Main Tread: Accept commands through TCP port 7000			*/
/*  2nd  Tread: Listen Multicast (SPREAD) messages and execute commands    */
/****************************************************************/

#define _DVSD
#define _DVSCMD
#include "dvsd.h"
#include "glo.h"

int (*call_vec[DVS_MAXCMDS])(dvs_cmd_t *dvscmd_ptr);

#define map(cmd_nr, handler) call_vec[(cmd_nr)] = (handler)

print_usage(void){
	fprintf(stderr,"Usage: dvsd <local_nodeid>\n");
	fprintf(stderr,"<local_nodeid> the LOCAL node ID\n");
	ERROR_EXIT(EDVSINVAL);	
}

double dwalltime()
{
	double sec;
	struct timeval tv;

	gettimeofday(&tv,NULL);
	sec = tv.tv_sec + tv.tv_usec/1000000.0;
	return sec;
}

int do_unused(dvs_cmd_t *dvscmd_ptr)
{
	dvscmd_ptr->dvs_paylen = 0;
	ERROR_PRINT(EDVSNOSYS);
}

int do_help(dvs_cmd_t *dvscmd_ptr)
{
	dvscmd_ptr->dvs_paylen = 0;
	return(OK);
}

/*===========================================================================*
 *				multicast_cmd				     *
 *===========================================================================*/
int multicast_cmd(dvs_cmd_t *dvscmd_ptr) 
{
	int rcode;

	// fill source node
	dvscmd_ptr->dvs_snode = local_nodeid;
	// multicast all nodes
   	USRDEBUG("dvscmd_ptr:" DVSRQST_FORMAT,DVSRQST_FIELDS(dvscmd_ptr));
	rcode = SP_multicast (dvsmbox, SAFE_MESS, (char *) group_name,
						dvscmd_ptr->dvs_cmd, sizeof(dvs_cmd_t)+dvscmd_ptr->dvs_paylen, 
						(char *) dvscmd_ptr);
	return(OK);
}

/*===========================================================================*
 *				tcp_connect				     *
 *===========================================================================*/
int tcp_connect(void){

    int ret;
    short int port_no;
    int optval = 1;

    port_no = DVSD_PORT;
	USRDEBUG("DVSD: running at port=%d\n", port_no);

    // Create server socket.
    if ( (dvsd_sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
        ERROR_EXIT(errno);

    if( (ret = setsockopt(dvsd_sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval))) < 0)
        ERROR_EXIT(errno);

    // Bind (attach) this process to the server socket.
    dvsd_addr.sin_family 		= AF_INET;
    dvsd_addr.sin_addr.s_addr 	= htonl(INADDR_ANY);
    dvsd_addr.sin_port 			= htons(port_no);
	ret = bind(dvsd_sd, (struct sockaddr *) &dvsd_addr, sizeof(dvsd_addr));
    if(ret < 0) ERROR_EXIT(errno);

	USRDEBUG("DVSD: is bound to port=%d socket=%d\n", port_no, dvsd_sd);

	ret = listen(dvsd_sd, 0);
    if(ret < 0) ERROR_EXIT(errno);
	

	return(OK);
}

/*===========================================================================*
 *				dvsd_init				     *
 *===========================================================================*/
int dvsd_init(void)
{
	int  dvsd_pid, i, rcode;
	
	last_rqst = time(NULL);

	dvsd_pid = getpid();
   	USRDEBUG( "dvsd_pid=%d\n",  dvsd_pid);
	
	rcode = dvk_open();
	if (rcode < 0)  ERROR_PRINT(rcode);
	
  	USRDEBUG( "Initialize the call vector to a safe default handler.\n");
  	for (i=0; i<DVS_MAXCMDS; i++) {
		USRDEBUG("Initilizing vector %d on address=%p\n",i, do_unused);
      	call_vec[i] = do_unused;
  	}

	init_nodes = 0;
	bm_init = 0;
	
	USRDEBUG("Initializing GCS\n");
	rcode = init_gcs();	
	if( rcode)ERROR_EXIT(rcode);
	USRDEBUG("Starting GCS thread\n");
	rcode = pthread_create( &gcs_thread, NULL, gcs_read_thread, 0 );
	if( rcode)ERROR_EXIT(rcode);
	
	pthread_mutex_lock(&dvs_mutex);
	pthread_cond_wait(&dvs_barrier,&dvs_mutex); /* unlock, wait, and lock again dvs_mutex */	
	USRDEBUG("DVSD has been signaled by the GCS thread  FSM_state=%X\n",  FSM_state);
	if( FSM_state == STS_LEAVE) {	/* An error occurs trying to join the spread group */
		pthread_mutex_unlock(&dvs_mutex);
		ERROR_RETURN(EDVSCONNREFUSED);
	}
	
	posix_memalign( (void**) &tcp_in_buf, getpagesize(), MAX_MESSLEN );
	if( tcp_in_buf == NULL) return (EDVSNOMEM);

	posix_memalign( (void**) &tcp_out_buf, getpagesize(), MAX_MESSLEN );
	if( tcp_out_buf == NULL) return (EDVSNOMEM);
	
	return(OK);
}

/****************************************************************************************/
/*			DVS DEAMON 						*/
/****************************************************************************************/
int  main ( int argc, char *argv[] )
{
	int rcode, cmd_nr, result, bytes, n;
	dvs_cmd_t *incmd_ptr, *outcmd_ptr;
	int sender_addrlen;
    char ip4[INET_ADDRSTRLEN];  // space to hold the IPv4 string
    struct sockaddr_in sa;
	
	if ( argc != 2 ) {
		print_usage();
 	    exit(1);
    }

	local_nodeid = atoi(argv[1]);
	USRDEBUG("DVSD: local_nodeid=%d\n", local_nodeid);

 	/* Initialize the DVS Deamon. */
	rcode = dvsd_init();
	if(rcode) ERROR_EXIT(rcode);
	
	rcode = tcp_connect();
	if( rcode) {	
		USRDEBUG("DVSD: tcp_connect rcode=%d\n",rcode);
		ERROR_EXIT(rcode);
	}

	incmd_ptr  = (dvs_cmd_t *) tcp_in_buf;
	outcmd_ptr = (dvs_cmd_t *) tcp_out_buf;

	/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
	/*	ANTETODO: el dvscmd no se sabe cual es el local_nodeid  */
	/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
	pthread_mutex_unlock(&dvs_mutex);
	while(TRUE) {

		do {
			sender_addrlen = sizeof(sa);
			USRDEBUG("DVSD: Waiting for connection.\n");
    		dvscmd_sd = accept(dvsd_sd, (struct sockaddr *) &sa, &sender_addrlen);
    		if(dvscmd_sd < 0) ERROR_PRINT(-errno);
		}while(dvscmd_sd < 0);
		
		USRDEBUG("DVSD: Remote sender [%s] connected on dvscmd_sd [%d]. Getting remote command.\n",
           		inet_ntop(AF_INET, &(sa.sin_addr), ip4, INET_ADDRSTRLEN), dvscmd_sd);
	
		USRDEBUG("DVSD is waiting for requests\n");
		bytes = recv(dvscmd_sd, tcp_in_buf, MAX_MESSLEN, 0 );
		USRDEBUG("DVSD recv bytes=%d\n", bytes);
		if(bytes < 0) { 
			ERROR_PRINT(-errno);
			close(dvscmd_sd);
			continue;
		}
		
		/*------------------------------------
		* Process the Request 
	 	*------------------------------------*/
		if( incmd_ptr->dvs_snode == DVS_LOCALNODE)
			incmd_ptr->dvs_snode = local_nodeid;
		if( incmd_ptr->dvs_dnode == DVS_LOCALNODE)
			incmd_ptr->dvs_dnode = local_nodeid;	
		USRDEBUG("DVSD REQUEST incmd_ptr:" DVSRQST_FORMAT,DVSRQST_FIELDS(incmd_ptr));
		cmd_nr = (unsigned) incmd_ptr->dvs_cmd;	
		USRDEBUG("DVSD cmd_nr=%d\n", cmd_nr);
		if (cmd_nr >= DVS_MAXCMDS || cmd_nr <= 0) {	/* check call number 	*/
			ERROR_RETURN(EDVSBADREQUEST);
			result = EDVSBADREQUEST;					/* illegal message type */
			bytes = sizeof(dvs_cmd_t);
      	} else {	
			USRDEBUG("DVSD: Calling vector %d\n",cmd_nr);
			pthread_mutex_lock(&dvs_mutex);
        	result = multicast_cmd(incmd_ptr);
			if( result < 0)	{
				ERROR_PRINT(result);
			}else {
				// wait CGS to receive the replay
				pthread_cond_wait(&dvs_barrier,&dvs_mutex); 
				USRDEBUG("DVSD has been signaled by the GCS thread FSM_state=%X\n",  FSM_state);
				bytes = sizeof(dvs_cmd_t) + outcmd_ptr->dvs_paylen;
				USRDEBUG("DVSD outcmd_ptr:" DVSRPLY_FORMAT, DVSRPLY_FIELDS(outcmd_ptr));
			}
			pthread_mutex_unlock(&dvs_mutex);
      	}
			
		/*------------------------------------
	 	* Send Reply to Client through TCP socket 
 		*------------------------------------*/
		USRDEBUG("REPLY outcmd_ptr:" DVSRPLY_FORMAT, DVSRPLY_FIELDS(outcmd_ptr));
       	n = send(dvscmd_sd, outcmd_ptr, bytes, 0);
		if( n < 0 ){
			ERROR_PRINT(-errno);
		}	
	}		
	
	// never here 
	if(rcode) ERROR_RETURN(rcode);
	return(OK);
 }





