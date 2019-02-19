/********************************************************/
/*  SPREAD PROXIES WITH BATCHING MESSAGES 		*/
/********************************************************/

#include "proxy.h"
#include "debug.h"
#include "macros.h"
#include "sp.h"

#define HEADER_SIZE sizeof(proxy_hdr_t)
#define BASE_PORT      3000

#define RETRY_US		2000 /* Microseconds */
#define BIND_RETRIES	3
#define BATCH_PENDING 	1
#define CMD_PENDING 	1

#define	   	STS_NEW				0x0000
#define		STS_WAIT_STATUS		0x0001
#define		STS_WAIT_INIT		0x0002
#define		STS_DISCONNECTED	0x0004
#define		STS_RUNNING			(MASK_RUNNING | MASK_INITIALIZED)
#define		STS_REQ_SLOTS		(MASK_REQUESTING | MASK_INITIALIZED)
#define		STS_MERGE_STATUS	(0x0020 | MASK_RUNNING |  MASK_INITIALIZED |  MASK_MERGING)

#define		STS_LEAVE 		STS_DISCONNECTED	
#define MAX_VSSETS      10

#define NO_PRIMARY_MBR  LOCALNODE
#define NULL_NODE  LOCALNODE

#define SPREAD_TIMEOUT_SEC	5
#define SPREAD_TIMEOUT_MSEC	0

#define MAX_MEMBERS     NR_NODES

int active_nodes;
int init_nodes;
int local_nodeid;
unsigned long int			bm_init;	
unsigned long int			bm_active;		
int		SP_bytes;		/* bytes returned by SP_receive */

unsigned long int dvs_nodes = 0;

mailbox px_mbox;		/* SPREAD MAILBOX  */	

char grp_name[] = "SP_PROXY";

char    Spread_name[80];
char    Private_group[MAX_GROUP_NAME];
char    first_name[MAX_GROUP_NAME];
char	User[80];

membership_info  memb_info;
vs_set_info      vssets[MAX_VSSETS];
unsigned int     my_vsset_index;
int              num_vs_sets;
char             members[MAX_MEMBERS][MAX_GROUP_NAME];
char		   sp_members[MAX_MEMBERS][MAX_GROUP_NAME];
int		   sp_nr_mbrs;

struct thread_desc_s {
    pthread_t 		td_thread;
	proxy_hdr_t 	*td_header;
	proxy_hdr_t 	*td_header2;
	proxy_hdr_t 	*td_header3;
	proxy_hdr_t 	*td_pseudo;
	proxy_payload_t *td_payload;	/* uncompressed payload 		*/	
	proxy_payload_t *td_payload2;
	proxy_payload_t *td_batch;
	int				td_batch_nr;

	pthread_mutex_t td_mtx;    /* mutex & condition to allow main thread to
								wait for the new thread to  set its TID */
	pthread_cond_t  td_cond;   /* '' */
	pid_t           td_tid;     /* to hold new thread's TID */
};
typedef struct thread_desc_s thread_desc_t;

thread_desc_t rdesc, *rtd_ptr;
thread_desc_t sdesc, *std_ptr;

#define  c_td_batch_nr		c_snd_seq
int	cmd_flag = 0;		// signals a command to be sent 
int rmsg_ok = 0;
int rmsg_fail = 0;
int smsg_ok = 0;
int smsg_fail = 0; 
int  FSM_state = 0;
int primary_mbr = 0; 
dvs_usr_t dvs;   
proxies_usr_t px, *px_ptr;

/*===========================================================================*
 *				get_nodeid				     
 * It converts the string provided by SPREAD into a node ID		      
 *===========================================================================*/
int get_nodeid(char *mbr_string)
{
	char *s_ptr;
	int nid;

	PXYDEBUG("mbr_string=%s\n", mbr_string);
	s_ptr = strchr(mbr_string, '.'); /* locate the dot character */
	if(s_ptr != NULL)
		*s_ptr = '\0';
	s_ptr = mbr_string;
	s_ptr++;	/* first character of node */
	nid = atoi(s_ptr);
	PXYDEBUG("member=%s nodeid=%d\n", mbr_string,  nid );
	return(nid);
}

	
/*===========================================================================*
 *				init_global_vars
 *===========================================================================*/
void init_global_vars()
{
	FSM_state 	= STS_NEW;	
	primary_mbr = NO_PRIMARY_MBR;
	init_nodes 	= 0;
	
	bm_init 	= 0;	/* which nodes are INITIALIZED 		*/
	bm_active	= 0;	/* which nodes are ACTIVE - 			*/
}

/*===========================================================================*
 *				init_spread				     
 *===========================================================================*/
int init_spread(void)
{
	int rcode;
#ifdef SPREAD_VERSION
    int     mver, miver, pver;
#endif
    sp_time test_timeout;

    test_timeout.sec = SPREAD_TIMEOUT_SEC;
    test_timeout.usec = SPREAD_TIMEOUT_MSEC;

	PXYDEBUG("group name=%s\n", grp_name);
	PXYDEBUG("user  name=%d.%d\n", local_nodeid, px.px_id);

#ifdef SPREAD_VERSION
    rcode = SP_version( &mver, &miver, &pver);
	if(!rcode)
        {
		SP_error (rcode);
	  	ERROR_PRINT("main: Illegal variables passed to SP_version()\n");
	  	ERROR_EXIT(rcode);
	}
	PXYDEBUG("Spread library version is %d.%d.%d\n", mver, miver, pver);
#else
    PXYDEBUG("Spread library version is %1.2f\n", SP_version() );
#endif
	/*------------------------------------------------------------------------------------
	* User:  it must be unique in the spread node.
	*  local_nodeid.dcid
	*--------------------------------------------------------------------------------------*/

	sprintf( Spread_name, "4803");
	sprintf( User, "%d.%d", local_nodeid, px.px_id);
	rcode = SP_connect_timeout( Spread_name, User , 0, 1, 
				&px_mbox, Private_group, test_timeout );
	if( rcode != ACCEPT_SESSION ) 	{
		SP_error (rcode);
		ERROR_EXIT(rcode);
	}
	PXYDEBUG("User %s: connected to %s with private group %s\n",
			User , Spread_name, Private_group);

	/*------------------------------------------------------------------------------------
	* Group name: PROXY 
	*--------------------------------------------------------------------------------------*/
	PXYDEBUG("Spread JOIN %s\n", grp_name);

	rcode = SP_join( px_mbox, grp_name);
	if( rcode){
		SP_error (rcode);
 		ERROR_EXIT(rcode);
	}

	init_global_vars();

	return(OK);
}


/*----------------------------------------------*/
/*      PROXY RECEIVER FUNCTIONS               */
/*----------------------------------------------*/
            
/*===========================================================================*
 *				spread_receive				     *
 *===========================================================================*/

int spread_receive(void)
{
	char	 sender[MAX_GROUP_NAME];
    char	 target_groups[MAX_MEMBERS][MAX_GROUP_NAME];
   	int		 num_groups;

	int		 service_type;
	int16	 	mess_type;
	int		 endian_mismatch;
	int		 i,j;
	int		 ret, mbr, nr_tmp;
	unsigned long int bm_tmp;


    while(1) {
		
		service_type = 0;
		num_groups = -1;
		PXYDEBUG("RPROXY Spread waiting a message\n");
		ret = SP_receive( px_mbox, &service_type, sender, 100, &num_groups, target_groups,
				&mess_type, &endian_mismatch, (sizeof(proxy_hdr_t)+ sizeof(proxy_payload_t)), (char *) rtd_ptr->td_header );
		if( ret < 0 ){
			if ( (ret == GROUPS_TOO_SHORT) || (ret == BUFFER_TOO_SHORT) ) {
				service_type = DROP_RECV;
				PXYDEBUG("\n========Buffers or Groups too Short=======\n");
				ret = SP_receive( px_mbox, &service_type, sender, 
						MAX_MEMBERS, &num_groups, target_groups,
						&mess_type, &endian_mismatch, (sizeof(proxy_hdr_t)+ sizeof(proxy_payload_t)), (char *) rtd_ptr->td_header );
			}
		}

		if (ret < 0 ) {
			SP_error( ret );
			if( ret == EDVSAGAIN){
				ERROR_PRINT(ret);
				continue;
			}
			ERROR_EXIT(ret);
		}else{
			SP_bytes = ret;
		}

		PXYDEBUG("sender=%s Private_group=%s service_type=%d SP_bytes=%d \n"
				,sender, Private_group, service_type, SP_bytes);

		if( Is_regular_mess( service_type ) )	{
			if     ( Is_unreliable_mess( service_type ) ) {PXYDEBUG("received UNRELIABLE \n ");}
			else if( Is_reliable_mess(   service_type ) ) {PXYDEBUG("received RELIABLE \n");}
			else if( Is_causal_mess(       service_type ) ) {PXYDEBUG("received CAUSAL \n");}
			else if( Is_agreed_mess(       service_type ) ) {PXYDEBUG("received AGREED \n");}
			else if( Is_safe_mess(   service_type ) || Is_fifo_mess(       service_type ) ) {
				PXYDEBUG("message from %s, of type %d, (endian %d) to %d groups (%d bytes)\n",
					sender, mess_type, endian_mismatch, num_groups, ret);
			if( rtd_ptr->td_header->c_dnode != local_nodeid)
				continue;
			PXYDEBUG("VALID PROXY MESSAGE RECEIVED from %s(%d bytes)\n",sender,ret);
			return(ret);
			}
		}else if( Is_membership_mess( service_type ) )	{
			ret = SP_get_memb_info((char *) rtd_ptr->td_header, service_type, &memb_info );
			if (ret < 0) {
				PXYDEBUG("BUG: membership message does not have valid body\n");
				SP_error( ret );
				ERROR_EXIT(ret);
			}

			if  ( Is_reg_memb_mess( service_type ) ) {
				PXYDEBUG("Received REGULAR membership for group %s with %d members, where I am member %d:\n",
					sender, num_groups, mess_type );

				if( Is_caused_join_mess( service_type ) ||
					Is_caused_leave_mess( service_type ) ||
					Is_caused_disconnect_mess( service_type ) ){
					bm_tmp = 0;
					memcpy((void*) sp_members, (void *) target_groups, num_groups*MAX_GROUP_NAME);
					for( i=0; i < num_groups; i++ ){
						mbr = get_nodeid(&sp_members[i][0]);
						PXYDEBUG("\t%s:%d\n", &sp_members[i][0], mbr );
						SET_BIT(bm_tmp, mbr);
	//					PXYDEBUG("grp id is %d %d %d\n",memb_info.gid.id[0], memb_info.gid.id[1], memb_info.gid.id[2] );
					}
					PXYDEBUG("num_groups=%d OLD bm_active=%X NEW bm_active=%X\n", 
						num_groups, bm_active, bm_tmp);				
				}
			}

			if( Is_caused_join_mess( service_type ) )	{
				/*----------------------------------------------------------------------------------------------------
				*   JOIN: The group has a new member
				*----------------------------------------------------------------------------------------------------*/
				PXYDEBUG("Due to the JOIN of %s\n",memb_info.changed_member );
				mbr = get_nodeid((char *)  memb_info.changed_member);
				active_nodes = num_groups;
				bm_active = bm_tmp;
			}else if( Is_caused_leave_mess( service_type ) 
				||  Is_caused_disconnect_mess( service_type ) ){
				/*----------------------------------------------------------------------------------------------------
				*   LEAVE or DISCONNECT:  A member has left the group
				*----------------------------------------------------------------------------------------------------*/
				PXYDEBUG("Due to the LEAVE or DISCONNECT of %s\n", memb_info.changed_member );
				mbr = get_nodeid((char *)  memb_info.changed_member);
				active_nodes = num_groups;
				bm_active = bm_tmp;	
			}else if( Is_caused_network_mess( service_type ) ){
				/*----------------------------------------------------------------------------------------------------
				*   NETWORK CHANGE:  A network partition or a dead deamon
				*----------------------------------------------------------------------------------------------------*/
				PXYDEBUG("Due to NETWORK change with %u VS sets\n", memb_info.num_vs_sets);
						num_vs_sets = SP_get_vs_sets_info( (char *) rtd_ptr->td_header, 
										&vssets[0], MAX_VSSETS, &my_vsset_index );
				if (num_vs_sets < 0) {
					PXYDEBUG("BUG: membership message has more then %d vs sets. Recompile with larger MAX_VSSETS\n", MAX_VSSETS);
					SP_error( num_vs_sets );
					ERROR_EXIT( num_vs_sets );
				}
				if (num_vs_sets == 0) {
					PXYDEBUG("BUG: membership message has %d vs_sets\n", 
						num_vs_sets);
					SP_error( num_vs_sets );
					ERROR_EXIT( EDVSGENERIC );
				}

				bm_tmp = 0;
				nr_tmp = 0;
				for( i = 0; i < num_vs_sets; i++ )  {
					PXYDEBUG("%s VS set %d has %u members:\n",
						(i  == my_vsset_index) ?("LOCAL") : ("OTHER"), 
						i, vssets[i].num_members );
					ret = SP_get_vs_set_members((char *) rtd_ptr->td_header, &vssets[i], members, MAX_MEMBERS);
					if (ret < 0) {
						PXYDEBUG("VS Set has more then %d members. Recompile with larger MAX_MEMBERS\n", MAX_MEMBERS);
						SP_error( ret );
						ERROR_EXIT( ret);
					}

					/*---------------------------------------------
					* get the bitmap of current members
					--------------------------------------------- */
					for( j = 0; j < vssets[i].num_members; j++ ) {
						PXYDEBUG("\t%s\n", members[j] );
						mbr = get_nodeid(members[j]);
						if(TEST_BIT(dvs_nodes, mbr) ){ 
							if(!TEST_BIT(bm_tmp, mbr)) {
								SET_BIT(bm_tmp, mbr);
								nr_tmp++;
							}
						}
					}
				}
				
				PXYDEBUG("OLD bm_active=%X active_nodes=%d\n", bm_active, active_nodes);
				PXYDEBUG("NEW bm_active=%X active_nodes=%d\n", bm_tmp, nr_tmp);

				if( bm_active > bm_tmp) {	/* a NETWORK PARTITION has occurred 	*/
					PXYDEBUG("NETWORK PARTITION has occurred\n");
					active_nodes = nr_tmp; 
					bm_active = bm_tmp;
				}else{
					if (bm_active < bm_tmp) {	/* a NETWORK MERGE has occurred 		*/
						PXYDEBUG("NETWORK MERGE has occurred\n");
						active_nodes = nr_tmp; 
						bm_active = bm_tmp;
					}else{
						PXYDEBUG("NETWORK CHANGE with no changed members!! ");
					}
				}
			}else if( Is_transition_mess(   service_type ) ) {
				PXYDEBUG("received TRANSITIONAL membership for group %s\n", sender );
				if( Is_caused_leave_mess( service_type ) ){
					PXYDEBUG("received membership message that left group %s\n", sender );
				}else {
					PXYDEBUG("received incorrecty membership message of type 0x%x\n", service_type );
				}
			} else if ( Is_reject_mess( service_type ) )      {
				PXYDEBUG("REJECTED message from %s, of servicetype 0x%x messtype %d, (endian %d) to %d groups \n(%d bytes): %s\n",
					sender, service_type, mess_type, endian_mismatch, num_groups, ret, rtd_ptr->td_header );
			}else {
				PXYDEBUG("received message of unknown message type 0x%x with ret %d\n", service_type, ret);
			}
		}
	}
}
			

/* pr_process_message: receives header and payload if any. Then deliver the 
 * message to local */
int pr_process_message(void) {
    int rcode, payload_size, i, ret;
 	message *m_ptr;
	proxy_hdr_t *bat_cmd, *bat_ptr;
	
	do {
		ret = spread_receive();
		if(ret < 0) {
			ERROR_PRINT(ret);
			sleep(1);
		}
	}while(rtd_ptr->td_header->c_cmd == CMD_NONE);

	bat_ptr = rtd_ptr->td_header;
	PXYDEBUG("RPROXY: " CMD_FORMAT, CMD_FIELDS(bat_ptr));

	/* now we have a proxy header in the buffer. Cast it.*/
	payload_size = rtd_ptr->td_header->c_len;
	PXYDEBUG("RPROXY: payload_size=%d\n", payload_size);

	switch(rtd_ptr->td_header->c_cmd){
		case CMD_SEND_MSG:
		case CMD_SNDREC_MSG:
		case CMD_REPLY_MSG:
			m_ptr = &rtd_ptr->td_header->c_u.cu_msg;
			PXYDEBUG("RPROXY: " MSG1_FORMAT,  MSG1_FIELDS(m_ptr));
			break;
		case CMD_COPYIN_DATA:
		case CMD_COPYOUT_DATA:
			PXYDEBUG("RPROXY: " VCOPY_FORMAT, VCOPY_FIELDS(rtd_ptr->td_header)); 
			break;
		default:
			break;
	}
		

	PXYDEBUG("RPROXY: put2lcl td_header=%lX\n", (unsigned long) rtd_ptr->td_header );
	rcode = dvk_put2lcl(rtd_ptr->td_header, (char *) rtd_ptr->td_payload);
	if( rcode < 0) ERROR_RETURN(rcode);	
	
	if( rtd_ptr->td_header->c_flags == FLAG_BATCHCMDS) {
		rtd_ptr->td_batch_nr = rtd_ptr->td_header->c_td_batch_nr;
		PXYDEBUG("RPROXY: rtd_ptr->td_batch_nr=%d\n", rtd_ptr->td_batch_nr);
		// check payload len
		if( (rtd_ptr->td_batch_nr * sizeof(proxy_hdr_t)) != rtd_ptr->td_header->c_len){
			PXYDEBUG("RPROXY: put2lcl\n");
			ERROR_RETURN(EDVSBADVALUE);
		}
		// put the batched commands
		bat_cmd = (proxy_hdr_t *) &rtd_ptr->td_payload->pay_cmd;
		for( i = 0; i < rtd_ptr->td_batch_nr; i++){
			bat_ptr = &bat_cmd[i];
			PXYDEBUG("RPROXY: bat_cmd[%d]:" CMD_FORMAT, i, CMD_FIELDS(bat_ptr));
			rcode = dvk_put2lcl(bat_ptr, (char *) rtd_ptr->td_payload);
			if( rcode < 0) ERROR_RETURN(rcode);		
		}
	}
	
    return(OK);    
}

/* pr_start_serving: accept connection from remote sender
   and loop receiving and processing messages
 */
void pr_start_serving(void)
{
    int rcode;

    while (1){

		rcode = dvk_proxy_conn(px.px_id, CONNECT_RPROXY);

    	/* Serve Forever */
		do { 
	       	/* get a complete message and process it */
       		rcode = pr_process_message();
       		if (rcode == OK) {
				PXYDEBUG("RPROXY: Message succesfully processed.\n");
				rmsg_ok++;
        	} else {
				PXYDEBUG("RPROXY: Message processing failure [%d]\n",rcode);
            			rmsg_fail++;
				if( rcode == EDVSNOTCONN) break;
			}	
		}while(1);

		rcode = dvk_proxy_conn(px.px_id, DISCONNECT_RPROXY);
   	}
      /* never reached */
}

/* pr_thread: creates socket */
void *pr_thread(void *nothing) 
{
    int rcode;
	/* Lock mutex... */
	pthread_mutex_lock(&rtd_ptr->td_mtx);
	/* Get and save TID and ready flag.. */
	rtd_ptr->td_tid = syscall(SYS_gettid);
	/* and signal main thread that we're ready */
	pthread_cond_signal(&rtd_ptr->td_cond);
	/* ..then unlock when we're done. */
	pthread_mutex_unlock(&rtd_ptr->td_mtx);
	;
	PXYDEBUG("RPROXY: Initializing proxy receiver. TID: %d\n", rtd_ptr->td_tid);
    
	do { 
		rcode = dvk_wait4bind_T(RETRY_US);
		PXYDEBUG("RPROXY: dvk_wait4bind_T  rcode=%d\n", rcode);
		if (rcode == EDVSTIMEDOUT) {
			PXYDEBUG("RPROXY: dvk_wait4bind_T TIMEOUT\n");
			continue ;
		}else if(rcode == NONE) { /* proxies have not endpoint */
			break;	
		} if( rcode < 0) 
			exit(EXIT_FAILURE);
	} while	(rcode < OK);
	
	posix_memalign( (void**) &rtd_ptr->td_header, getpagesize(), (sizeof(proxy_hdr_t)+ sizeof(proxy_payload_t)));
	if (rtd_ptr->td_header== NULL) {
    		perror("rtd_ptr->td_header posix_memalign");
    		exit(1);
  	}

	posix_memalign( (void**) &rtd_ptr->td_payload, getpagesize(), (sizeof(proxy_payload_t)));
	if (rtd_ptr->td_payload== NULL) {
    		perror("rtd_ptr->td_payload posix_memalign");
    		exit(1);
  	}

	posix_memalign( (void**) &rtd_ptr->td_payload2, getpagesize(), (sizeof(proxy_hdr_t)));
	if (rtd_ptr->td_payload2== NULL) {
    		perror("rtd_ptr->td_payload2 posix_memalign");
    		exit(1);
  	}
	
	posix_memalign( (void**) &rtd_ptr->td_batch, getpagesize(), (sizeof(proxy_payload_t)));
	if (rtd_ptr->td_batch== NULL) {
    		perror("rtd_ptr->td_batch posix_memalign");
    		exit(1);
  	}
	
	PXYDEBUG("td_header=%p td_payload=%p diff=%d\n", 
			rtd_ptr->td_header, rtd_ptr->td_payload, ((char*) rtd_ptr->td_payload - (char*) rtd_ptr->td_header));

   	pr_start_serving();
	return(NULL);
}

/*----------------------------------------------*/
/*      PROXY SENDER FUNCTIONS                  */
/*----------------------------------------------*/

/* ps_send_header: send  header to remote receiver */
int  ps_send_header(proxy_hdr_t *ptr_hdr ) 
{
    int hdrlen, rcode ;

	hdrlen = sizeof(proxy_hdr_t); // how many bytes we have left to send
	PXYDEBUG("SPROXY: send hdrlen=%d group:%s cmd=%d \n", hdrlen, grp_name, ptr_hdr->c_cmd);

	rcode = SP_multicast (px_mbox, FIFO_MESS, 
				grp_name,
				ptr_hdr->c_cmd,  
				hdrlen,
				(char*)ptr_hdr);
	if(rcode <0) {
		SP_error (rcode);
		ERROR_RETURN(rcode);
	} 
    return(OK);
}

/* ps_send_hdr_pay: send payload to remote receiver */
int  ps_send_hdr_pay(proxy_hdr_t *ptr_hdr, proxy_payload_t *ptr_pay ) 
{
    int hdrlen, rcode;
	char *ptr; 

	hdrlen = sizeof(proxy_hdr_t); // how many bytes we have left to send
	PXYDEBUG("SPROXY: send hdrlen=%d paylen=%d group:%s cmd=%d \n", 
			hdrlen, ptr_hdr->c_len, grp_name, ptr_hdr->c_cmd);

	ptr = (char *) ptr_hdr;
	ptr += hdrlen;
	memcpy( ptr, ptr_pay, ptr_hdr->c_len);
	
	rcode = SP_multicast (px_mbox, FIFO_MESS, 
				grp_name,
				ptr_hdr->c_cmd,  
				(hdrlen+ptr_hdr->c_len),
				(char*)ptr_hdr);
	if(rcode <0) {
		SP_error (rcode);
		ERROR_RETURN(rcode);
	} 
	
    return(OK);
}

/* 
 * ps_send_remote: send a message (header + payload if existing) 
 * to remote receiver
 */
int  ps_send_remote(proxy_hdr_t *ptr_hdr, proxy_payload_t *ptr_pay ) 
{
	int rcode;

	PXYDEBUG("SPROXY:" CMD_FORMAT,CMD_FIELDS(ptr_hdr));

	if( ptr_hdr->c_len == 0) {
		/* send the header */
		rcode =  ps_send_header(ptr_hdr);
		if ( rcode != OK)  ERROR_RETURN(rcode);
	}else {
		PXYDEBUG("SPROXY: send header+payload paylen=%d\n", ptr_hdr->c_len );
		rcode =  ps_send_hdr_pay( ptr_hdr, ptr_pay);
		if ( rcode != OK)  ERROR_RETURN(rcode);
	}
	
    return(OK);
}

/* 
 * ps_start_serving: gets local message and sends it to remote receiver .
 * Do this forever.
 */
int  ps_start_serving(void)
{
	proxy_hdr_t *bat_vect;
    int rcode;
    message *m_ptr;
    int ret;

    while(1) {
		cmd_flag = 0;
		std_ptr->td_batch_nr = 0;		// count the number of batching commands in the buffer
			
		PXYDEBUG("SPROXY %d: Waiting a message\n", std_ptr->td_tid);
		ret = dvk_get2rmt(std_ptr->td_header, std_ptr->td_payload);            
		switch(ret) {
			case OK:
				break;
			case EDVSTIMEDOUT:
				PXYDEBUG("SPROXY: Sending HELLO \n");
				std_ptr->td_header->c_cmd = CMD_NONE;
				std_ptr->td_header->c_len = 0;
				std_ptr->td_header->c_rcode = 0;
				rcode =  ps_send_remote(std_ptr->td_header, std_ptr->td_payload);
				cmd_flag = 0;
				if (rcode == 0) smsg_ok++;
				else smsg_fail++;
				break;
			case EDVSNOTCONN:
				ERROR_RETURN(EDVSNOTCONN);
			default:
				ERROR_RETURN(ret);
				break;
		}
		if( ret == EDVSTIMEDOUT) continue;

		PXYDEBUG("SPROXY: %d "HDR_FORMAT,std_ptr->td_tid, HDR_FIELDS(std_ptr->td_header)); 

#ifdef USE_BATCHING			
		//------------------------ BATCHEABLE COMMAND -------------------------------
		if(  (std_ptr->td_header->c_cmd != CMD_COPYIN_DATA )     // is a batcheable command ??
		  && (std_ptr->td_header->c_cmd != CMD_COPYOUT_DATA)){	// YESSSSS

			if ( (std_ptr->td_header->c_cmd == CMD_SEND_MSG) 
				||(std_ptr->td_header->c_cmd == CMD_SNDREC_MSG)
				||(std_ptr->td_header->c_cmd == CMD_REPLY_MSG)){
				m_ptr = &std_ptr->td_header->c_u.cu_msg;
				PXYDEBUG("SPROXY: " MSG1_FORMAT,  MSG1_FIELDS(m_ptr));
			}
			// store original header into batched header 
			memcpy( (char*) std_ptr->td_header3, std_ptr->td_header, sizeof(proxy_hdr_t));
			bat_vect = (proxy_hdr_t*) std_ptr->td_batch;		
			do{
				PXYDEBUG("SPROXY %d: Getting more messages\n", std_ptr->td_tid);
				ret = dvk_get2rmt_T(std_ptr->td_header2, std_ptr->td_payload2, TIMEOUT_NOWAIT);            
				if( ret != OK) {
					switch(ret) {
						case EDVSTIMEDOUT:
						case EDVSAGAIN:
							ERROR_PRINT(ret); 
							break;
						case EDVSNOTCONN: 
							ERROR_RETURN(EDVSNOTCONN);
							break;
						default:
							ERROR_PRINT(ret);
							break;
					}
					break;
				} 
				PXYDEBUG("SPROXY: %d "HDR_FORMAT,std_ptr->td_tid, HDR_FIELDS(std_ptr->td_header)); 

				if(  (std_ptr->td_header2->c_cmd == CMD_COPYIN_DATA )    // is a batcheable command ??
					|| (std_ptr->td_header2->c_cmd == CMD_COPYOUT_DATA)){// NOOOOOOO
					cmd_flag = CMD_PENDING;
					break;
				}
			
				// Get another command 
				memcpy( (char*) &bat_vect[std_ptr->td_batch_nr], (char*) std_ptr->td_header2, sizeof(proxy_hdr_t));
				std_ptr->td_batch_nr++;		
				PXYDEBUG("SPROXY: new BATCHED COMMAND std_ptr->td_batch_nr=%d\n", std_ptr->td_batch_nr);		

			}while ( std_ptr->td_batch_nr < MAXBATCMD);
		}
		
		if( std_ptr->td_batch_nr > 0) { 			// is batching in course??	
			PXYDEBUG("SPROXY: sending BATCHED COMMANDS std_ptr->td_batch_nr=%d\n", std_ptr->td_batch_nr);
			std_ptr->td_header3->c_flags = (std_ptr->td_batch_nr)?FLAG_BATCHCMDS:0;
			std_ptr->td_header3->c_td_batch_nr = std_ptr->td_batch_nr;
			std_ptr->td_header3->c_len = std_ptr->td_batch_nr * sizeof(proxy_hdr_t);
			rcode =  ps_send_remote(std_ptr->td_header3, std_ptr->td_batch);
			if (rcode == 0) smsg_ok++;
			else smsg_fail++;
			std_ptr->td_batch_nr = 0;		// count the number of batching commands in the buffer
		} else {
#endif //USE_BATCHING			

			rcode =  ps_send_remote(std_ptr->td_header, std_ptr->td_payload);
			if (rcode == 0) smsg_ok++;
			else smsg_fail++;	
			
#ifdef USE_BATCHING			
		}
		// now, send the non batcheable command plus payload 
		if( cmd_flag == CMD_PENDING) { 
			rcode =  ps_send_remote(std_ptr->td_header2, std_ptr->td_payload2);
			cmd_flag = 0;
			if (rcode == 0) smsg_ok++;
			else smsg_fail++;
		}
#endif //USE_BATCHING			

	}

    /* never reached */
    exit(1);
}

int ps_connect_to_remote()
{
	int rcode;

	PXYDEBUG("SPROXY: Initializing on TID:%d\n", std_ptr->td_tid);

	init_global_vars();
	return(OK);
}

/* 
 * ps_thread: creates sender socket, the connect to remote and
 * start sending messages to remote 
 */
void *ps_thread(void *nothing) 
{
    int rcode = 0;

	/* Lock mutex... */
	pthread_mutex_lock(&std_ptr->td_mtx);
	/* Get and save TID and ready flag.. */
	std_ptr->td_tid = syscall(SYS_gettid);
	/* and signal main thread that we're ready */
	pthread_cond_signal(&std_ptr->td_cond);
	/* ..then unlock when we're done. */
	pthread_mutex_unlock(&std_ptr->td_mtx);
	
	PXYDEBUG("SPROXY: Initializing on TID:%d\n", std_ptr->td_tid);
    
	do { 
		rcode = dvk_wait4bind_T(RETRY_US);
		PXYDEBUG("SPROXY: dvk_wait4bind_T  rcode=%d\n", rcode);
		if (rcode == EDVSTIMEDOUT) {
			PXYDEBUG("SPROXY: dvk_wait4bind_T TIMEOUT\n");
			continue ;
		}else if(rcode == NONE) { /* proxies have not endpoint */
			break;	
		} if( rcode < 0) 
			exit(EXIT_FAILURE);
	} while	(rcode < OK);
		
	posix_memalign( (void**) &std_ptr->td_header, getpagesize(), (sizeof(proxy_hdr_t)+ sizeof(proxy_payload_t)));
	if (std_ptr->td_header== NULL) {
    		perror("std_ptr->td_header posix_memalign");
    		exit(1);
  	}

	posix_memalign( (void**) &std_ptr->td_header2, getpagesize(), (sizeof(proxy_hdr_t)));
	if (std_ptr->td_header2 == NULL) {
    		perror("std_ptr->td_header2 posix_memalign");
    		exit(1);
  	}

	posix_memalign( (void**) &std_ptr->td_header3, getpagesize(), (sizeof(proxy_hdr_t)));
	if (std_ptr->td_header3== NULL) {
    		perror("std_ptr->td_header3 posix_memalign");
    		exit(1);
  	}
	
	posix_memalign( (void**) &std_ptr->td_payload, getpagesize(), (sizeof(proxy_payload_t)));
	if (std_ptr->td_payload== NULL) {
    		perror("std_ptr->td_payload posix_memalign");
    		exit(1);
  	}

	posix_memalign( (void**) &std_ptr->td_payload2, getpagesize(), (sizeof(proxy_payload_t)));
	if (std_ptr->td_payload == NULL) {
    		perror("std_ptr->td_payload2 posix_memalign");
    		exit(1);
  	}
	
	posix_memalign( (void**) &std_ptr->td_batch, getpagesize(), (sizeof(proxy_payload_t)));
	if (std_ptr->td_batch == NULL) {
    		perror("std_ptr->td_batch posix_memalign");
    		exit(1);
  	}
	

	/* try to connect many times */
	std_ptr->td_batch_nr = 0;		// number of batching commands
	cmd_flag = 0;		// signals a command to be sent 
	while(1) {
	
		rcode = dvk_proxy_conn(px.px_id, CONNECT_SPROXY);
		if(rcode)ERROR_EXIT(rcode);
		
		ps_start_serving();
	
		rcode = dvk_proxy_conn(px.px_id, DISCONNECT_SPROXY);
		if(rcode) ERROR_EXIT(rcode);
		 	
		/* ps_connect_to_remote: connects to the remote receiver */
		rcode = ps_connect_to_remote(); 
		if(rcode) ERROR_EXIT(rcode); 
	}
    /* code never reaches here */
	return(NULL);

}

extern int errno;

/*----------------------------------------------*/
/*		MAIN: 			*/
/*----------------------------------------------*/
int  main ( int argc, char *argv[] )
{
    int pid;
    int ret;
    dvs_usr_t *d_ptr;    

    if (argc != 3) {
     	fprintf(stderr,"Usage: %s <px_name> <px_id> \n", argv[0]);
    	exit(0);
    }

    strncpy(px.px_name,argv[1], MAXPROXYNAME);
	px.px_name[MAXPROCNAME-1]= '\0';
    printf("SPREAD Proxy Pair name: %s\n",px.px_name);
 
    px.px_id = atoi(argv[2]);
    printf("Proxy Pair id: %d\n",px.px_id);

	ret = dvk_open();
	if (ret < 0)  ERROR_PRINT(ret);
	
    local_nodeid = dvk_getdvsinfo(&dvs);
    printf("local_nodeid=%d\n",local_nodeid);

    d_ptr=&dvs;
	PXYDEBUG(DVS_USR_FORMAT,DVS_USR_FIELDS(d_ptr));

    pid = getpid();
	PXYDEBUG("MAIN: pid=%d local_nodeid=%d\n", pid, local_nodeid);
	
	init_spread();

    /* creates SENDER and RECEIVER Proxies as Treads */
	PXYDEBUG("MAIN: pthread_create RPROXY\n");
	rtd_ptr = &rdesc;
	pthread_cond_init(&rtd_ptr->td_cond, NULL);  /* init condition */
	pthread_mutex_init(&rtd_ptr->td_mtx, NULL);  /* init mutex */
	pthread_mutex_lock(&rtd_ptr->td_mtx);
	if ( (ret = pthread_create(&rtd_ptr->td_thread, NULL, pr_thread,(void*)NULL )) != 0) {
		ERROR_EXIT(ret);
	}
    pthread_cond_wait(&rtd_ptr->td_cond, &rtd_ptr->td_mtx);
	PXYDEBUG("MAIN: RPROXY td_tid=%d\n", rtd_ptr->td_tid);
    pthread_mutex_unlock(&rtd_ptr->td_mtx);

	PXYDEBUG("MAIN: pthread_create SPROXY\n");
	std_ptr = &sdesc;
	pthread_cond_init(&std_ptr->td_cond, NULL);  /* init condition */
	pthread_mutex_init(&std_ptr->td_mtx, NULL);  /* init mutex */
	pthread_mutex_lock(&std_ptr->td_mtx);
	if ((ret = pthread_create(&std_ptr->td_thread, NULL, ps_thread,(void*)NULL )) != 0) {
		ERROR_EXIT(ret);
	}
    pthread_cond_wait(&std_ptr->td_cond, &std_ptr->td_mtx);
	PXYDEBUG("MAIN: SPROXY td_tid=%d\n", std_ptr->td_tid);
    pthread_mutex_unlock(&std_ptr->td_mtx);
	
    /* register the proxies */
    ret = dvk_proxies_bind(px.px_name, px.px_id, std_ptr->td_tid, rtd_ptr->td_tid, MAXCOPYBUF);
    if( ret < 0) ERROR_EXIT(ret);
	
	px_ptr = &px;
	PXYDEBUG(PX_USR_FORMAT , PX_USR_FIELDS(px_ptr));
	PXYDEBUG("bound to (%d,%d)\n", sdesc.td_tid, rdesc.td_tid);
//	sleep(60);

	ret= dvk_node_up(px.px_name, px.px_id, px.px_id);	
	
	pthread_join ( std_ptr->td_thread, NULL );
	pthread_join ( rtd_ptr->td_thread, NULL );
	
	pthread_mutex_destroy(&std_ptr->td_mtx);
	pthread_cond_destroy(&std_ptr->td_cond);
	
	pthread_mutex_destroy(&rtd_ptr->td_mtx);
	pthread_cond_destroy(&rtd_ptr->td_cond);
    
	exit(0);
}

