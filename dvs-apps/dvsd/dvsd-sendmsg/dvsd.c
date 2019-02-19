/****************************************************************/
/* 				DVSD				*/
/*  The DVS Deamon which allow DVS cluster management 			*/
/*  Main Tread: Accept commands through Message queue		*/
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
	ERROR_EXIT(EMOLINVAL);	
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
	ERROR_PRINT(EMOLNOSYS);
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
   	DVSDDEBUG("dvscmd_ptr:" DVSCMD_FORMAT,DVSCMD_FIELDS(dvscmd_ptr));
	rcode = SP_multicast (dvsmbox, SAFE_MESS, (char *) group_name,
						dvscmd_ptr->dvs_cmd, sizeof(dvs_cmd_t)+dvscmd_ptr->dvs_paylen, 
						(char *) dvscmd_ptr);
	// wait CGS to receive the replay
	pthread_cond_wait(&dvs_barrier,&dvs_mutex); 
	DVSDDEBUG("DVSD has been signaled by the GCS thread FSM_state=%X\n",  FSM_state);
	return(OK);
}

/*===========================================================================*
 *				udp_connect				     *
 *===========================================================================*/
int udp_connect(void){
	
	int ret;
    int optval = 1;
	
	// Create server socket.
    if ( (dvsd_sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
        ERROR_EXIT(errno);

    if( (ret = setsockopt(dvsd_sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval))) < 0)
       	ERROR_EXIT(errno);

    // Bind (attach) this process to the server socket.
    dvsd_addr.sin_family = AF_INET;
    dvsd_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dvsd_addr.sin_port = htons(port_no);
   	ret = bind(dvsd_sd, (struct sockaddr *) &dvsd_addr, sizeof(dvsd_addr));
    if(ret < 0) ERROR_EXIT(errno);

	SVRDEBUG("DVSD: is bound to port=%d socket=%d\n", port_no, dvsd_sd);
	
	posix_memalign( (void**) &cmd_ptr, getpagesize(), sizeof(dvs_cmd_t) );
	if( cmd_ptr == NULL) return (EMOLNOMEM);
	
	posix_memalign( (void**) &udp_in_buf, getpagesize(), MAX_MESSLEN );
	if( udp_in_buf == NULL) return (EMOLNOMEM);

	posix_memalign( (void**) &udp_out_buf, getpagesize(), MAX_MESSLEN );
	if( udp_out_buf == NULL) return (EMOLNOMEM);
	
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
   	DVSDDEBUG( "dvsd_pid=%d\n",  dvsd_pid);
	
  	DVSDDEBUG( "Initialize the call vector to a safe default handler.\n");
  	for (i=0; i<DVS_MAXCMDS; i++) {
		DVSDDEBUG("Initilizing vector %d on address=%p\n",i, do_unused);
      	call_vec[i] = do_unused;
  	}

	init_nodes = 0;
	bm_init = 0;
	
	DVSDDEBUG("Initializing GCS\n");
	rcode = init_gcs();	
	if( rcode)ERROR_EXIT(rcode);
	DVSDDEBUG("Starting GCS thread\n");
	rcode = pthread_create( &gcs_thread, NULL, gcs_read_thread, 0 );
	if( rcode)ERROR_EXIT(rcode);
	
	pthread_mutex_lock(&dvs_mutex);
	pthread_cond_wait(&dvs_barrier,&dvs_mutex); /* unlock, wait, and lock again dvs_mutex */	
	DVSDDEBUG("DVSD has been signaled by the GCS thread  FSM_state=%X\n",  FSM_state);
	if( FSM_state == STS_LEAVE) {	/* An error occurs trying to join the spread group */
		pthread_mutex_unlock(&dvs_mutex);
		ERROR_RETURN(EMOLCONNREFUSED);
	}
		
	return(OK);
}

/****************************************************************************************/
/*			DVS DEAMON 						*/
/****************************************************************************************/
int  main ( int argc, char *argv[] )
{
	int rcode, cmd_nr, result, bytes;
	dvs_cmd_t *incmd_ptr, *outcmd_ptr;
	struct msghdr rqst_msg;
	struct msghdr rply_msg;
	
	if ( argc != 2 ) {
		print_usage();
 	    exit(1);
    }

	local_nodeid = atoi(argv[1]);
	DVSDDEBUG("local_nodeid=%d\n", local_nodeid);

 	/* Initialize the DVS Deamon. */
	rcode = dvsd_init();
	if(rcode) ERROR_EXIT(rcode);
	
	rcode = udp_connect();
	if( rcode) {	
		DVSDDEBUG("udp_connect rcode=%d\n",rcode);
		ERROR_EXIT(rcode);
	}

	incmd_ptr  = (dvs_cmd_t *) udp_in_buf;
	outcmd_ptr = (dvs_cmd_t *) udp_out_buf;

	/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
	/*	ANTETODO: el dvscmd no se sabe cual es el local_nodeid  */
	/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
	pthread_mutex_unlock(&dvs_mutex);
	while(TRUE) {

		/*------------------------------------
		* Receive Request from Client through UDP
	 	*------------------------------------*/
		
		DVSDDEBUG("DVSD is waiting for requests\n");
		cmd_ptr = (dvs_cmd_t *) udp_in_buf;
		rqst_iov[0].iov_base=cmd_ptr;
		rqst_iov[0].iov_len=sizeof(dvs_cmd_t);
		rqst_msg.msg_name=&src_addr;
		rqst_msg.msg_namelen=sizeof(src_addr);
		rqst_msg.msg_iov=iov;
		rqst_msg.msg_iovlen=1;
		rqst_msg.msg_control=0;
		rqst_msg.msg_controllen=0;
		bytes =recvmsg(dvsd_sd,&rply_msg,0);
		if (bytes==-1) {
			DVSERR(-errno);
		} else if (rply_msg.msg_flags&MSG_TRUNC) {
			DVSDDEBUG("datagram too large for buffer: truncated");
		}
	
		count=recvmsg(fd,&message,0);
		if (count==-1) {
		die("%s",strerror(errno));
		} else if (message.msg_flags&MSG_TRUNC) {
		warn("datagram too large for buffer: truncated");
		} else {
		handle_datagram(buffer,count);
		}

		
		
		
		
		
		
		
		bytes = msgrcv(mq_in, udp_in_buf, MAX_MESSLEN, 0 , 0 );
		DVSDDEBUG("msgrcv bytes=%d\n", bytes);
		if(bytes < 0) {
			DVSERR(errno);
			exit(1);
		}
		
		/*------------------------------------
		* Process the Request 
	 	*------------------------------------*/
   		DVSDDEBUG("RECEIVE incmd_ptr:"DVSCMD_FORMAT,DVSCMD_FIELDS(incmd_ptr));
		cmd_nr = (unsigned) incmd_ptr->dvs_cmd;	
		DVSDDEBUG("cmd_nr=%d\n", cmd_nr);
		if (cmd_nr >= DVS_MAXCMDS || cmd_nr <= 0) {	/* check call number 	*/
			DVSERR(EMOLBADREQUEST);
			result = EMOLBADREQUEST;					/* illegal message type */
			bytes = sizeof(dvs_cmd_t);
      	} else {	
			DVSDDEBUG("Calling vector %d\n",cmd_nr);
			pthread_mutex_lock(&dvs_mutex);
        	result = multicast_cmd(incmd_ptr);
			bytes = sizeof(dvs_cmd_t);
			if( result == OK)
				bytes += outcmd_ptr->dvs_paylen;
			DVSDDEBUG("outcmd_ptr:" DVSCMD_FORMAT, DVSCMD_FIELDS(outcmd_ptr));
			pthread_mutex_unlock(&dvs_mutex);
      	}
			
		/*------------------------------------
	 	* Send Reply to Client through msgq 
 		*------------------------------------*/
		DVSDDEBUG("REPLY outcmd_ptr:" DVSCMD_FORMAT, DVSCMD_FIELDS(outcmd_ptr));
		bytes = sizeof(dvs_cmd_t) + outcmd_ptr->dvs_paylen;
		bytes = msgsnd(mq_out, udp_out_buf, bytes, 0); 
		DVSDDEBUG("msgsnd bytes=%d\n", bytes);
		if(bytes < 0) {
			DVSERR(errno);
			exit(1);
		}
		
	}		
	
	// never here 
	if(rcode) ERROR_RETURN(rcode);
	return(OK);
 }





