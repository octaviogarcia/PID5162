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
 *				msgq_init				     *
 *===========================================================================*/
int msgq_init(void){
	
	mq_in = msgget(QUEUEBASE, IPC_CREAT | 0x660);
	if ( mq_in < 0) {
		if ( errno != EEXIST) {
			DVSERR(mq_in);
			return(mq_in);
		}
		DVSDDEBUG( "The queue with key=%d already exists\n",QUEUEBASE);
		mq_in = msgget( (QUEUEBASE), 0);
		if(mq_in < 0) {
			DVSERR(mq_in);
			return(mq_in);
		}
		DVSDDEBUG("msgget OK\n");
	} 

	msgctl(mq_in , IPC_STAT, &mq_in_ds);
	DVSDDEBUG("before mq_in msg_qbytes =%d\n",mq_in_ds.msg_qbytes);
	mq_in_ds.msg_qbytes = MAX_MESSLEN;
	msgctl(mq_in , IPC_SET, &mq_in_ds);
	msgctl(mq_in , IPC_STAT, &mq_in_ds);
	DVSDDEBUG("after mq_in msg_qbytes =%d\n",mq_in_ds.msg_qbytes);

	mq_out = msgget(QUEUEBASE+1, IPC_CREAT | 0x660);
	if ( mq_out < 0) {
		if ( errno != EEXIST) {
			DVSERR(mq_out);
			return(mq_out);
		}
		DVSDDEBUG("The queue with key=%d already exists\n",QUEUEBASE+1);
		mq_out = msgget( (QUEUEBASE+1), 0);
		if(mq_out < 0) {
			DVSERR(mq_out);
			return(mq_out);
		}
		DVSDDEBUG("msgget OK\n");
	}

	msgctl(mq_out , IPC_STAT, &mq_out_ds);
	DVSDDEBUG("before mq_out msg_qbytes =%d\n",mq_out_ds.msg_qbytes);
	mq_out_ds.msg_qbytes = MAX_MESSLEN;
	msgctl(mq_out , IPC_SET, &mq_out_ds);
	msgctl(mq_out , IPC_STAT, &mq_out_ds);
	DVSDDEBUG("after mq_out msg_qbytes =%d\n",mq_out_ds.msg_qbytes);
	
	posix_memalign( (void**) &mq_in_buf, getpagesize(), sizeof(struct msgbuf_s) );
	if( mq_in_buf == NULL) return (EMOLNOMEM);

	posix_memalign( (void**) &mq_out_buf, getpagesize(), sizeof(struct msgbuf_s) );
	if( mq_out_buf == NULL) return (EMOLNOMEM);
	
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
	
	if ( argc != 2 ) {
		print_usage();
 	    exit(1);
    }

	local_nodeid = atoi(argv[1]);
	DVSDDEBUG("local_nodeid=%d\n", local_nodeid);

 	/* Initialize the DVS Deamon. */
	rcode = dvsd_init();
	if(rcode) ERROR_EXIT(rcode);
	
	rcode = msgq_init();
	if( rcode) {	
		DVSDDEBUG("msgq_init rcode=%d\n",rcode);
		ERROR_EXIT(rcode);
	}

	incmd_ptr  = (dvs_cmd_t *) &mq_in_buf->mtext;
	outcmd_ptr = (dvs_cmd_t *) &mq_out_buf->mtext;

	/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
	/*	ANTETODO: el dvscmd no se sabe cual es el local_nodeid  */
	/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
	
	
	pthread_mutex_unlock(&dvs_mutex);
	while(TRUE) {

		/*------------------------------------
		* Receive Request from Client through msgq
	 	*------------------------------------*/
		DVSDDEBUG("DVSD is waiting for requests\n");
		mq_in_buf->mtype = 0x0001;
		bytes = msgrcv(mq_in, mq_in_buf, MAX_MESSLEN, 0 , 0 );
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
		mq_out_buf->mtype = 0x0001;
		bytes = msgsnd(mq_out, mq_out_buf, bytes, 0); 
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





