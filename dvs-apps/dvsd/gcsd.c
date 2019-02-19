/****************************************************************/
/* 				GCS				*/
/****************************************************************/

#include "dvsd.h"
#include "glo.h"

#define TRUE 1

/*===========================================================================*
 *				get_init_nodes				     
 *===========================================================================*/
int get_init_nodes(void)
{
	int init_nr, i;

	init_nr = 0;
	for( i=0; i < sizeof(bm_init)*8; i++ ) {
		if( TEST_BIT(bm_init, i)){
			init_nr++;
		}
	}
	
	USRDEBUG("init_nr=%d\n", init_nr);
	return(init_nr);
}

/*===========================================================================*
 *				get_nodeid				     
 * It converts the string provided by SPREAD into a node ID
*  name format "#node.X#nodeX" 
 *===========================================================================*/
int get_nodeid(char *mbr_string)
{
	char *s_ptr, *d_ptr;
	int nid;
	USRDEBUG("mbr_string: %s\n", mbr_string);

	s_ptr = strchr(mbr_string, '.'); /* locate the dot character */
	if(s_ptr == NULL){
		USRDEBUG("Member name has not have a dot (format NODE.nodeid): %s\n", mbr_string);
		exit(EDVSBADNODEID);
	}	
	*s_ptr = '\0';
	s_ptr++;	/* first character of node */

	d_ptr = strchr(mbr_string, '#'); /* locate the # character */
	if(d_ptr == NULL){
		USRDEBUG("Member name has not have a dot (format #node.X#nodeX): %s\n", mbr_string);
		exit(EDVSBADNODEID);
	}	
	*d_ptr = '\0';
	
	nid = atoi(s_ptr);
	USRDEBUG("member=%s nodeid=%d\n", mbr_string,  nid );
	return(nid);
}
	
/*===========================================================================*
 *				init_global_vars
 *===========================================================================*/
int init_global_vars()
{
	FSM_state 	= STS_NEW;	
	init_nodes 	= 0;
	bm_init 	= 0;	/* which nodes are INITIALIZED 		Replicated	*/
}

/*===========================================================================*
 *				gcs_dvsinit				     
 *===========================================================================*/
int gcs_dvsinit(dvs_cmd_t *cmd_ptr)
{
	int ret;
	char *ptr;
	dvs_usr_t *dvs_ptr;
		
	USRDEBUG("cmd_ptr:" DVSRQST_FORMAT,DVSRQST_FIELDS(cmd_ptr));
	
	// set the pointer to payload: dvs_usr_t 
	ptr = (char *) cmd_ptr;
	ptr += sizeof(dvs_cmd_t);
	dvs_ptr = (dvs_usr_t*) ptr;
	
	USRDEBUG(DVS_USR_FORMAT, DVS_USR_FIELDS(dvs_ptr));
	USRDEBUG(DVS_MAX_FORMAT, DVS_MAX_FIELDS(dvs_ptr));
	USRDEBUG(DVS_VER_FORMAT, DVS_VER_FIELDS(dvs_ptr))

	ret = dvk_dvs_init(local_nodeid, dvs_ptr);

	cmd_ptr->dvs_paylen = 0;
	cmd_ptr->dvs_lines = 0;
			
	if(ret != local_nodeid) ERROR_RETURN(ret);
	return(ret);
}

/*===========================================================================*
 *				gcs_dvsend				     
 *===========================================================================*/
int gcs_dvsend(dvs_cmd_t *cmd_ptr)
{
	int ret;
	dvs_usr_t  *dvs_ptr;
	
	USRDEBUG("cmd_ptr:" DVSRQST_FORMAT,DVSRQST_FIELDS(cmd_ptr));
	
	ret = dvk_dvs_end();
	
	cmd_ptr->dvs_paylen = 0;
	cmd_ptr->dvs_lines = 0;
	if(ret) ERROR_RETURN(ret);
	return(ret);
}

/*===========================================================================*
 *				gcs_dvsinfo				     
 *===========================================================================*/
int gcs_dvsinfo(dvs_cmd_t *cmd_ptr)
{
	int ret;
	dvs_usr_t  *dvs_ptr;
	char *ptr;
		
	USRDEBUG("cmd_ptr:" DVSRQST_FORMAT,DVSRQST_FIELDS(cmd_ptr));

	// pointer position to dvs_usr_t in spread buffer.
	ptr = (char *) cmd_ptr;
	ptr += sizeof(dvs_cmd_t);
	dvs_ptr = (dvs_usr_t  *) ptr;
		
	ret = dvk_getdvsinfo(dvs_ptr);
	if( ret != local_nodeid) {
		cmd_ptr->dvs_lines 	= 0;
		cmd_ptr->dvs_paylen = 0;
		ERROR_RETURN(ret);
	}
	
	USRDEBUG(DVS_USR_FORMAT, DVS_USR_FIELDS(dvs_ptr));
	USRDEBUG(DVS_MAX_FORMAT, DVS_MAX_FIELDS(dvs_ptr));
	USRDEBUG(DVS_VER_FORMAT, DVS_VER_FIELDS(dvs_ptr))
	
	// set cmd fields correctly
	cmd_ptr->dvs_lines = 1;
	cmd_ptr->dvs_paylen = sizeof(dvs_usr_t);
	return(ret);
}

/*===========================================================================*
 *				gcs_dcinfo				     
 *===========================================================================*/
int gcs_dcinfo(dvs_cmd_t *cmd_ptr)
{
	int ret;
	dc_usr_t  *dc_ptr;
	char *ptr;
		
	USRDEBUG("cmd_ptr:" DVSRQST_FORMAT,DVSRQST_FIELDS(cmd_ptr));

	// pointer position to dvs_usr_t in spread buffer.
	ptr = (char *) cmd_ptr;
	ptr += sizeof(dvs_cmd_t);
	dc_ptr = (dc_usr_t *) ptr;
	
	ret = dvk_getdcinfo(cmd_ptr->dvs_arg1, dc_ptr);
    if( ret != OK) {
		cmd_ptr->dvs_lines 	= 0;
		cmd_ptr->dvs_paylen = 0;
		ERROR_RETURN(ret);
	}

	USRDEBUG(DC_USR1_FORMAT, DC_USR1_FIELDS(dc_ptr));
	USRDEBUG(DC_USR2_FORMAT, DC_USR2_FIELDS(dc_ptr));

	// set cmd fields correctly
	cmd_ptr->dvs_lines = 1;
	cmd_ptr->dvs_paylen = sizeof(dc_usr_t);
	return(ret);
}

/*===========================================================================*
 *				gcs_nodeinfo				     
 *===========================================================================*/
int gcs_nodeinfo(dvs_cmd_t *cmd_ptr)
{
	int ret;
	node_usr_t *node_ptr;
	char *ptr;
		
	USRDEBUG("cmd_ptr:" DVSRQST_FORMAT,DVSRQST_FIELDS(cmd_ptr));

	// pointer position to dvs_usr_t in spread buffer.
	ptr = (char *) cmd_ptr;
	ptr += sizeof(dvs_cmd_t);
	node_ptr = (node_usr_t*) ptr;
	
	ret = dvk_getnodeinfo(cmd_ptr->dvs_arg1, node_ptr);
    if( ret != OK) {
		cmd_ptr->dvs_lines 	= 0;
		cmd_ptr->dvs_paylen = 0;
		ERROR_RETURN(ret);
	}
	
	printf( NODE_USR_FORMAT, NODE_USR_FIELDS(node_ptr));
	printf( NODE_TIME_FORMAT, NODE_TIME_FIELDS(node_ptr));

	// set cmd fields correctly
	cmd_ptr->dvs_lines = 1;
	cmd_ptr->dvs_paylen = sizeof(node_usr_t);
	return(ret);
}

/*===========================================================================*
 *				gcs_proxyinfo				     
 *===========================================================================*/
int gcs_proxyinfo(dvs_cmd_t *cmd_ptr)
{
	int ret;
	proc_usr_t *sproxy_ptr;
	proc_usr_t *rproxy_ptr;
	proxies_usr_t *px_ptr;
	char *ptr;
		
	USRDEBUG("cmd_ptr:" DVSRQST_FORMAT,DVSRQST_FIELDS(cmd_ptr));

	// pointer position to dvs_usr_t in spread buffer.
	ptr = (char *) cmd_ptr;
	ptr += sizeof(dvs_cmd_t);
	px_ptr = (proxies_usr_t*) ptr;

	ptr += sizeof(proxies_usr_t);
	sproxy_ptr = (proc_usr_t*) ptr;

	ptr += sizeof(proc_usr_t);
	rproxy_ptr = (proc_usr_t*) ptr;
	
	ret = dvk_getproxyinfo(cmd_ptr->dvs_arg1, sproxy_ptr, rproxy_ptr);
    if( ret != OK) {
		cmd_ptr->dvs_lines 	= 0;
		cmd_ptr->dvs_paylen = 0;
		ERROR_RETURN(ret);
	}
	
	USRDEBUG( PROC_USR_FORMAT, PROC_USR_FIELDS(sproxy_ptr));
	USRDEBUG( PROC_WAIT_FORMAT, PROC_WAIT_FIELDS(sproxy_ptr));
	USRDEBUG( PROC_COUNT_FORMAT, PROC_COUNT_FIELDS(sproxy_ptr));
	
	USRDEBUG( PROC_USR_FORMAT, PROC_USR_FIELDS(rproxy_ptr));
	USRDEBUG( PROC_WAIT_FORMAT, PROC_WAIT_FIELDS(rproxy_ptr));
	USRDEBUG( PROC_COUNT_FORMAT, PROC_COUNT_FIELDS(rproxy_ptr));

	// set cmd fields correctly
	cmd_ptr->dvs_lines = 3;
	cmd_ptr->dvs_paylen = sizeof(proxies_usr_t) + (2*(sizeof(proc_usr_t)));
	return(ret);
}

/*===========================================================================*
 *				gcs_procinfo				     
 *===========================================================================*/
int gcs_procinfo(dvs_cmd_t *cmd_ptr)
{
	int ret;
	proc_usr_t *proc_ptr;
	char *ptr;
		
	USRDEBUG("cmd_ptr:" DVSRQST_FORMAT,DVSRQST_FIELDS(cmd_ptr));

	// pointer position to dvs_usr_t in spread buffer.
	ptr = (char *) cmd_ptr;
	ptr += sizeof(dvs_cmd_t);
	proc_ptr = (proc_usr_t*) ptr;
	
	ret = dvk_getprocinfo(cmd_ptr->dvs_arg1, cmd_ptr->dvs_arg2, proc_ptr);
	if( ret != OK) {
		cmd_ptr->dvs_lines 	= 0;
		cmd_ptr->dvs_paylen = 0;
		ERROR_RETURN(ret);
	}
	
	USRDEBUG( PROC_USR_FORMAT, PROC_USR_FIELDS(proc_ptr));
	USRDEBUG( PROC_WAIT_FORMAT, PROC_WAIT_FIELDS(proc_ptr));
	USRDEBUG( PROC_COUNT_FORMAT, PROC_COUNT_FIELDS(proc_ptr));

	// set cmd fields correctly
	cmd_ptr->dvs_lines = 1;
	cmd_ptr->dvs_paylen = sizeof(proc_usr_t);
	return(ret);
}

/*===========================================================================*
 *				init_gcs				     
 * It connects GCS thread to the SPREAD daemon and initilize several local
 * and replicated variables
 *===========================================================================*/
int init_gcs(void)
{
	int rcode, i;
#ifdef SPREAD_VERSION
    int     mver, miver, pver;
#endif
    sp_time test_timeout;

    test_timeout.sec = SLOT_TIMEOUT_SEC;
    test_timeout.usec = SLOT_TIMEOUT_MSEC;

	USRDEBUG("group name=DVS\n");
	USRDEBUG("user  name=DVS.%d\n", local_nodeid);

#ifdef SPREAD_VERSION
    rcode = SP_version( &mver, &miver, &pver);
	if(!rcode)
        {
		SP_error (rcode);
	  	ERROR_RETURN("main: Illegal variables passed to SP_version()\n");
	  	ERROR_EXIT(rcode);
	}
	USRDEBUG("Spread library version is %d.%d.%d\n", mver, miver, pver);
#else
    USRDEBUG("Spread library version is %1.2f\n", SP_version() );
#endif
	/*------------------------------------------------------------------------------------
	* User:  it must be unique in the spread node.
	*--------------------------------------------------------------------------------------*/
	sprintf( Spread_name, "4803");
	sprintf( User, "node.%d", local_nodeid);
	sleep(2); // wait the start of spread deamon 
	rcode = SP_connect_timeout( Spread_name, User , 0, 1, 
				&dvsmbox, Private_group, test_timeout );
	if( rcode != ACCEPT_SESSION ) 	{
		SP_error (rcode);
		ERROR_EXIT(rcode);
	}
	USRDEBUG("User %s: connected to %s with private group %s\n",
			User , Spread_name, Private_group);

	
	init_global_vars();
	
	/*------------------------------------------------------------------------------------
	* Group name: dc_name
	*--------------------------------------------------------------------------------------*/
	rcode = SP_join( dvsmbox, group_name);
	if( rcode){
		SP_error (rcode);
 		ERROR_EXIT(rcode);
	}

	return(OK);
}

/*===========================================================================*
 *				sp_join											 *
 * A NEW member has joint the DC group but it is not initialized
 *===========================================================================*/
int sp_join(int new_mbr)
{
	int i, rcode;

	USRDEBUG("FSM_state=%X new_member=%d init_nodes=%d bm_init=%X\n", 
		FSM_state, new_mbr, init_nodes, bm_init);
	
	if( new_mbr == local_nodeid){		/* The own JOIN command	*/
			/* it is ready to start running */
			FSM_state = STS_RUNNING;
	}
	
	/* add the node to the initilized nodes bitmap */
	SET_BIT(bm_init, local_nodeid);
	init_nodes++;

	/* Wake up DVSD */
	USRDEBUG("Wake up dvsd: new_mbr=%d\n", new_mbr);
	pthread_cond_signal(&dvs_barrier);	/* Wakeup DVSD 	*/

	return(OK);
}

/*===========================================================================*
 *				sp_disconnect				
 * A member has left or it has disconnected from the DC group	
 *===========================================================================*/
int sp_disconnect(int  disc_mbr)
{
	USRDEBUG("FSM_state=%X disc_mbr=%d init_nodes=%d\n",FSM_state, disc_mbr, init_nodes);
	if(local_nodeid == disc_mbr) {
		FSM_state = STS_DISCONNECTED;
		return(OK);
	}
	
	/* if the dead node was initialized */
	if( TEST_BIT(bm_init, disc_mbr)) 
		CLR_BIT(bm_init, disc_mbr);
	
	USRDEBUG("disc_mbr=%d init_nodes=%d \n",disc_mbr, init_nodes, bm_init);
	return(OK);
}

/*-----------------------------------------------------------------------
*				sp_net_partition
*  A network partition has occurred
*------------------------------------------------------------------------*/
int sp_net_partition(void)
{
	USRDEBUG("FSM_state=%X\n", FSM_state);

	/* mask the old bitmaps with the mask of active members 	*/
	/* only the active nodes should be considered 			 	*/
	USRDEBUG("PART OLD init_nodes=%d bm_init=%X\n", init_nodes , bm_init);
	init_nodes = get_init_nodes();
	USRDEBUG("PART NEW init_nodes=%d bm_init=%X\n", init_nodes , bm_init);

	return(OK);
}

/*----------------------------------------------------------------------------------------------------
*				sp_net_merge
*  A network merge has occurred
*  the Primary member of each merged partition
*  broadcast its current process slot table (PST)
*  only filled with the gcs owned by members of their partitions
*----------------------------------------------------------------------------------------------------*/
int sp_net_merge(void)
{
	USRDEBUG("FSM_state=%X\n", FSM_state);

	/* mask the old bitmaps with the mask of active members 	*/
	/* only the active nodes should be considered 			 	*/
	USRDEBUG("MERGE OLD init_nodes=%d bm_init=%X\n", init_nodes , bm_init);
	init_nodes = get_init_nodes();
	USRDEBUG("MERGE NEW init_nodes=%d bm_init=%X\n", init_nodes , bm_init);

	return(OK);
}

/*===========================================================================*
 *				gcs_read_thread				     *
 *===========================================================================*/

void *gcs_read_thread(void *arg)
{
	int rcode, mtype;
	int sp_pid;
	static 	char source[MAX_GROUP_NAME];
	dvs_cmd_t  sp_msg, *sp_ptr;

	sp_ptr =&sp_msg;
	
	while(TRUE){
		gcs_loop(&mtype, source);
	}
}

/*===========================================================================*
 *				gcs_loop				     *
 * return : service_type
 *
 *===========================================================================*/

int gcs_loop(int *mtype, char *source)
{
	char	sender[MAX_GROUP_NAME];
    char	target_groups[MAX_MEMBERS][MAX_GROUP_NAME];
   	int		num_groups;

	int		service_type;
	int16	gcsmsg_type;
	int		endian_mismatch;
	int		i,j;
	int		ret, mbr, nr_tmp;
	unsigned long int bm_tmp;

	dvs_cmd_t  *sp_ptr;
	char  *s_ptr;

	service_type 	= 0;
	num_groups 		= -1;

replay:

	ret = SP_receive( dvsmbox, &service_type, sender, 100, &num_groups, target_groups,
			&gcsmsg_type, &endian_mismatch, sizeof(gcsmsg_in), gcsmsg_in );
	if( ret < 0 ){
       	if ( (ret == GROUPS_TOO_SHORT) || (ret == BUFFER_TOO_SHORT) ) {
			service_type = DROP_RECV;
            USRDEBUG("\n========Buffers or Groups too Short=======\n");
            ret = SP_receive( dvsmbox, &service_type, sender, 
					MAX_MEMBERS, &num_groups, target_groups,
					&gcsmsg_type, &endian_mismatch, sizeof(gcsmsg_in), gcsmsg_in );
		}
	}

	if (ret < 0 ) {
		SP_error( ret );
		if( ret == EDVSAGAIN){
			ERROR_PRINT(ret);
			sleep(5);
			goto replay;
		}
		ERROR_EXIT(ret);
	}else{
		SP_bytes = ret;
	}

	USRDEBUG("sender=%s Private_group=%s service_type=%d SP_bytes=%d \n"
			,sender, Private_group, service_type, SP_bytes);

	sp_ptr = (dvs_cmd_t *) gcsmsg_in;

	pthread_mutex_lock(&dvs_mutex);	/* protect global variables */
	if( Is_regular_mess( service_type ) )	{
		gcsmsg_in[ret] = 0;
		if     ( Is_unreliable_mess( service_type ) ) {USRDEBUG("received UNRELIABLE \n ");}
		else if( Is_reliable_mess(   service_type ) ) {USRDEBUG("received RELIABLE \n");}
		else if( Is_causal_mess(       service_type ) ) {USRDEBUG("received CAUSAL \n");}
		else if( Is_agreed_mess(       service_type ) ) {USRDEBUG("received AGREED \n");}
		else if( Is_safe_mess(   service_type ) || Is_fifo_mess(       service_type ) ) {
			USRDEBUG("command from %s, of gcsmsg_type=%d, (endian %d) to %d groups (%d bytes)\n",
				sender, gcsmsg_type, endian_mismatch, num_groups, ret);
			
			USRDEBUG("sp_ptr:" DVSRQST_FORMAT,DVSRQST_FIELDS(sp_ptr));

			// check if destination is DVS_ALLNODES
			if( sp_ptr->dvs_dnode != DVS_ALLNODES){
				// check if sender is a GCS member
				USRDEBUG("bm_init=%X dvs_snode=%d \n", bm_init, sp_ptr->dvs_snode );
				if( !TEST_BIT(bm_init, sp_ptr->dvs_snode)){
					ret = EDVSBADNODEID;
					goto loop_unlock;
				}
				// Check if the command was sent to this node
				USRDEBUG("dvs_dnode=%d local_nodeid=%d\n", sp_ptr->dvs_dnode, local_nodeid );			
				if( sp_ptr->dvs_dnode != local_nodeid){
					ret = OK;	// ignore command
					goto loop_unlock;
				}
			}

			if( gcsmsg_type & DVS_ACKNOWLEDGE) {
				USRDEBUG("DVS_ACKNOWLEDGE gcsmsg_type=%X\n", gcsmsg_type);
				switch(gcsmsg_type){
					case DVS_DVSINIT_ACK:
					case DVS_DVSEND_ACK:
					case DVS_DVSINFO_ACK:
					case DVS_DCINFO_ACK:
					case DVS_NODEINFO_ACK:
					case DVS_PROXYINFO_ACK:
					case DVS_PROCINFO_ACK:
						break;
					default:
						USRDEBUG("Unknown command ACK type %X\n", gcsmsg_type);
						*mtype = gcsmsg_type;
						ret = OK;
						break;
				}
				// Copy received data to DVSD
				sp_ptr->dvs_bmnodes = init_nodes;
				memcpy ( tcp_out_buf, sp_ptr, sizeof(dvs_cmd_t)+sp_ptr->dvs_paylen);
				// Wakeup DVSD 
				USRDEBUG("Wake up DVSD\n");
				pthread_cond_signal(&dvs_barrier);	/* Wakeup DVSD */
				ret = OK;
			} else {
				USRDEBUG("DVS COMMAND gcsmsg_type=%X\n", gcsmsg_type);
				switch(gcsmsg_type){
					case DVS_DVSINIT:
						ret = gcs_dvsinit(sp_ptr);
						break;
					case DVS_DVSEND:
						ret = gcs_dvsend(sp_ptr);
						break;
					case DVS_DVSINFO:
						ret = gcs_dvsinfo(sp_ptr);
						break;
					case DVS_DCINFO:
						ret = gcs_dcinfo(sp_ptr);
						break;
					case DVS_NODEINFO:
						ret = gcs_nodeinfo(sp_ptr);
						break;
					case DVS_PROXYINFO:
						ret = gcs_proxyinfo(sp_ptr);
						break;
					case DVS_PROCINFO:
						ret = gcs_procinfo(sp_ptr);
						break;
					default:
						USRDEBUG("Unknown command type %X\n", gcsmsg_type);
						*mtype = gcsmsg_type;
						ret = OK;
						break;
				}
				// Reply to requester
				sp_ptr->dvs_cmd  |= DVS_ACKNOWLEDGE;			
				sp_ptr->dvs_dnode = sp_ptr->dvs_snode;
				sp_ptr->dvs_snode = local_nodeid;
				sp_ptr->dvs_bmnodes = init_nodes;
				sp_ptr->dvs_rcode  = ret;
				USRDEBUG("sp_ptr reply:" DVSRPLY_FORMAT,DVSRPLY_FIELDS(sp_ptr));
				ret = SP_multicast (dvsmbox, SAFE_MESS, (char *) group_name,
							sp_ptr->dvs_cmd, 
							sizeof(dvs_cmd_t)+sp_ptr->dvs_paylen,
							(char *) sp_ptr);
			}
		}
	}else if( Is_membership_mess( service_type ) )	{
        ret = SP_get_memb_info( gcsmsg_in, service_type, &memb_info );
        if (ret < 0) {
			USRDEBUG("BUG: membership command does not have valid body\n");
           	SP_error( ret );
			pthread_mutex_unlock(&dvs_mutex);
         	ERROR_EXIT(ret);
        }

		if  ( Is_reg_memb_mess( service_type ) ) {
			USRDEBUG("Received REGULAR membership for group %s with %d members, where I am member %d:\n",
				sender, num_groups, gcsmsg_type );

			if( Is_caused_join_mess( service_type ) ||
				Is_caused_leave_mess( service_type ) ||
				Is_caused_disconnect_mess( service_type ) ){
				bm_tmp = 0;
				memcpy((void*) sp_members, (void *) target_groups, num_groups*MAX_GROUP_NAME);
				for( i=0; i < num_groups; i++ ){
					mbr = get_nodeid(&sp_members[i][0]);
					USRDEBUG("\t%s:%d\n", &sp_members[i][0], mbr );
					SET_BIT(bm_tmp, mbr);
//					USRDEBUG("grp id is %d %d %d\n",memb_info.gid.id[0], memb_info.gid.id[1], memb_info.gid.id[2] );
				}
				USRDEBUG("num_groups=%d OLD bm_init=%X NEW bm_init=%X\n", 
					num_groups, bm_init, bm_tmp);				
			}
		}

		if( Is_caused_join_mess( service_type ) )	{
			/*----------------------------------------------------------------------------------------------------
			*   JOIN: The group has a new member
			*----------------------------------------------------------------------------------------------------*/
			USRDEBUG("Due to the JOIN of %s\n",memb_info.changed_member );
			mbr = get_nodeid((char *)  memb_info.changed_member);
			init_nodes = num_groups;
			bm_init = bm_tmp;
			ret = sp_join(mbr);
		}else if( Is_caused_leave_mess( service_type ) 
			||  Is_caused_disconnect_mess( service_type ) ){
			/*----------------------------------------------------------------------------------------------------
			*   LEAVE or DISCONNECT:  A member has left the group
			*----------------------------------------------------------------------------------------------------*/
			USRDEBUG("Due to the LEAVE or DISCONNECT of %s\n", memb_info.changed_member );
			mbr = get_nodeid((char *)  memb_info.changed_member);
			init_nodes = num_groups;
			bm_init = bm_tmp;	
			ret = sp_disconnect(mbr);
		}else if( Is_caused_network_mess( service_type ) ){
			/*----------------------------------------------------------------------------------------------------
			*   NETWORK CHANGE:  A network partition or a dead deamon
			*----------------------------------------------------------------------------------------------------*/
			USRDEBUG("Due to NETWORK change with %u VS sets\n", memb_info.num_vs_sets);
            		num_vs_sets = SP_get_vs_sets_info( gcsmsg_in, 
									&vssets[0], MAX_VSSETS, &my_vsset_index );
            if (num_vs_sets < 0) {
				USRDEBUG("BUG: membership command has more then %d vs sets. Recompile with larger MAX_VSSETS\n", MAX_VSSETS);
				SP_error( num_vs_sets );
				pthread_mutex_unlock(&dvs_mutex);
               	ERROR_EXIT( num_vs_sets );
			}
            if (num_vs_sets == 0) {
				USRDEBUG("BUG: membership command has %d vs_sets\n", 
					num_vs_sets);
				SP_error( num_vs_sets );
				pthread_mutex_unlock(&dvs_mutex);
               	ERROR_EXIT( EDVSGENERIC );
			}

			bm_tmp = 0;
			nr_tmp = 0;
            for( i = 0; i < num_vs_sets; i++ )  {
				USRDEBUG("%s VS set %d has %u members:\n",
					(i  == my_vsset_index) ?("LOCAL") : ("OTHER"), 
					i, vssets[i].num_members );
               	ret = SP_get_vs_set_members(gcsmsg_in, &vssets[i], members, MAX_MEMBERS);
               	if (ret < 0) {
					USRDEBUG("VS Set has more then %d members. Recompile with larger MAX_MEMBERS\n", MAX_MEMBERS);
					SP_error( ret );
					pthread_mutex_unlock(&dvs_mutex);
                   	ERROR_EXIT( ret);
              	}

				/*---------------------------------------------
				* get the bitmap of current members
				--------------------------------------------- */
				for( j = 0; j < vssets[i].num_members; j++ ) {
					USRDEBUG("\t%s\n", members[j] );
					mbr = get_nodeid(members[j]);
					if(!TEST_BIT(bm_tmp, mbr)) {
						SET_BIT(bm_tmp, mbr);
						nr_tmp++;
					}
				}
			}
			
			USRDEBUG("OLD bm_init=%X init_nodes=%d\n", bm_init, init_nodes);
			USRDEBUG("NEW bm_init=%X init_nodes=%d\n", bm_tmp, nr_tmp);

			if( bm_init > bm_tmp) {	/* a NETWORK PARTITION has occurred 	*/
				USRDEBUG("NETWORK PARTITION has occurred\n");
				init_nodes = nr_tmp; 
				bm_init = bm_tmp;
				sp_net_partition();
			}else{
				if (bm_init < bm_tmp) {	/* a NETWORK MERGE has occurred 		*/
					USRDEBUG("NETWORK MERGE has occurred\n");
					init_nodes = nr_tmp; 
					bm_init = bm_tmp;
					sp_net_merge();
				}else{
					USRDEBUG("NETWORK CHANGE with no changed members!! ");
				}
			}
		}else if( Is_transition_mess(   service_type ) ) {
			USRDEBUG("received TRANSITIONAL membership for group %s\n", sender );
			if( Is_caused_leave_mess( service_type ) ){
				USRDEBUG("received membership command that left group %s\n", sender );
			}else {
				USRDEBUG("received incorrecty membership command of type 0x%x\n", service_type );
			}
		} else if ( Is_reject_mess( service_type ) )      {
			USRDEBUG("REJECTED command from %s, of servicetype 0x%x messtype %d, (endian %d) to %d groups \n(%d bytes): %s\n",
				sender, service_type, gcsmsg_type, endian_mismatch, num_groups, ret, gcsmsg_in );
		}else {
			USRDEBUG("received command of unknown command type 0x%x with ret %d\n", service_type, ret);
		}
	}	
loop_unlock:	
	pthread_mutex_unlock(&dvs_mutex);
	if(ret < 0) ERROR_RETURN(ret);
	return(ret);
}

