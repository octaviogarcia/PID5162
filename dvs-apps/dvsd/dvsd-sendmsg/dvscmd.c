/****************************************************************/
/* 				DVSCMD				*/
/* COMMAND LINE INTERFACE */
/****************************************************************/

#define _DVSD
#include "dvsd.h"

int (*call_vec[DVS_MAXCMDS])(int argc, char *argv[]);
#define map(cmd_nr, handler) call_vec[(cmd_nr)] = (handler)

dvs_usr_t dvs_lcl = {
		.d_nr_dcs=NR_DCS,
		.d_nr_nodes=NR_NODES,
		.d_nr_procs=NR_PROCS,
		.d_nr_tasks=NR_TASKS,
		.d_nr_sysprocs=NR_SYS_PROCS,

		.d_max_copybuf=MAXCOPYBUF,
		.d_max_copylen=MAXCOPYLEN,

		.d_dbglvl=0x00000000, /* DEFAULT IS 520966 = 0X7F306 */
		2,
		1
		};

print_usage(void){
	fprintf(stderr,"Usage: dvscmd <dvscmd> [<commands args ...>]\n");
	fprintf(stderr,"DVS Command\n");
	fprintf(stderr,"DVSINIT	<nodeid> [-C nr_dcs] [-N nr_nodes] [-P nr_procs] [-T nr_tasks]"
						" [-B max_copybuf] [-L max_copylen] \n" 
						" [-S nr_sysprocs] [-D dbglvl]\n");
	fprintf(stderr,"DVSINFO	<nodeid> \n");
	fprintf(stderr,"DVSEND	<nodeid> \n");
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

int cmd_unused(int argc, char *argv[])
{
	ERROR_PRINT(EMOLNOSYS);
}

/*===========================================*
 *				cmd_dvsinit				     *
 *===========================================*/
int cmd_dvsinit(int argc, char *argv[]) 
{
	int nodeid, bytes, c;
	dvs_usr_t  *dvs_ptr;
	char *ptr;
	struct msghdr rqst_msg, *rqst_ptr;
	struct msghdr rply_msg, *rply_ptr;
    int n, len, total, maxdata, received = 0;
	
	DVSDDEBUG("DVSINIT argc=%d\n", argc);

	while ((c = getopt(argc, argv, "C:N:P:T:S:B:L:D:")) != -1) {
		switch(c) {
			case 'C':
				dvs_lcl.d_nr_dcs = atoi(optarg);
				if( dvs_lcl.d_nr_dcs <= 0 || dvs_lcl.d_nr_dcs  > NR_DCS) {
					fprintf (stderr, "Invalid nr_dcs [1-%d]\n", NR_DCS);
					exit(1);
				}
				break;
			case 'N':
				dvs_lcl.d_nr_nodes = atoi(optarg);
				if(dvs_lcl.d_nr_nodes <= 0 || dvs_lcl.d_nr_nodes  > NR_NODES ) {
					fprintf (stderr, "Invalid nr_nodes [1-%d]\n", dvs_lcl.d_nr_nodes );
					exit(1);
				}
				break;
			case 'P':
				dvs_lcl.d_nr_procs = atoi(optarg);
				if( dvs_lcl.d_nr_procs <= 0 || dvs_lcl.d_nr_procs > NR_PROCS ) {
					fprintf (stderr, "Invalid nr_procs [1-%d]\n", NR_PROCS);
					exit(1);
				}
				break;					
			case 'T':
				dvs_lcl.d_nr_tasks = atoi(optarg);
				if( dvs_lcl.d_nr_tasks <= 0 || dvs_lcl.d_nr_tasks  > NR_TASKS ) {
					fprintf (stderr, "Invalid nr_tasks [1-%d]\n", NR_TASKS);
					exit(1);
				}
				break;	
			case 'S':
				dvs_lcl.d_nr_sysprocs = atoi(optarg);
				if( dvs_lcl.d_nr_sysprocs <= 0 || dvs_lcl.d_nr_sysprocs > NR_SYS_PROCS ) {
					fprintf (stderr, "Invalid nr_sysprocs [1-%d]\n", NR_SYS_PROCS);
					exit(1);
				}
				break;

			case 'B':
				dvs_lcl.d_max_copybuf = atoi(optarg);
				if( dvs_lcl.d_max_copybuf <= 0 || dvs_lcl.d_max_copybuf > MAXCOPYBUF ) {
					fprintf (stderr, "Invalid 0 < max_copybuf < %d\n", MAXCOPYBUF);
					exit(1);
				}
				break;
			case 'L':
				dvs_lcl.d_max_copylen = atoi(optarg);
				if( dvs_lcl.d_max_copylen <= 0 || dvs_lcl.d_max_copylen > MAXCOPYLEN ) {
					fprintf (stderr, "Invalid 0 < max_copylen < %d\n", MAXCOPYLEN);
					exit(1);
				}
				break;
			case 'D':
				dvs_lcl.d_dbglvl = atol(optarg);
				break;
			case 'Q':
				dvs_lcl.d_dbglvl = atol(optarg);
				break;
			default:
				print_usage();
				exit(1);
		}
	}

	nodeid = atoi(argv[2]);
	if (nodeid < 0 || nodeid > MAX_MEMBERS){
		fprintf(stderr,"Invalid nodeid (%d)\n", nodeid);
		ERROR_EXIT(EMOLBADNODEID)
	}
	DVSDDEBUG("DVSINIT nodeid=%d\n", nodeid);

	// ------------- REQUEST  -----------------
	// the command is passed in the request IOV
	// the argument/data is passed in the ancillary data 
	
	// set the pointer to payload: dvs_usr_t 
	out_aux_ptr = (struct cmsghdr *) udp_out_buf;
	ptr = (char *) out_aux_ptr;
	ptr += sizeof(struct cmsghdr);
	dvs_ptr = (dvs_usr_t*) ptr;
	
	// fill ancillary data 
	out_aux_ptr->cmsg_len   = sizeof(struct cmsghdr) + sizeof(dvs_usr_t));
	out_aux_ptr->cmsg_level = 0;
	out_aux_ptr->cmsg_type  = DVS_DVSINIT;
	DVSDDEBUG("DVSINIT REQUEST out_aux_ptr:" MSGAUX_FORMAT, MSGAUX_FIELDS(out_aux_ptr));

	// copy from local DVS user struct to msgq message payload
	memcpy(dvs_ptr, &dvs_lcl, sizeof(dvs_usr_t));
	DVSDDEBUG("DVSINIT REQUEST " DVS_USR_FORMAT, DVS_USR_FIELDS(dvs_ptr));
	DVSDDEBUG("DVSINIT REQUEST " DVS_MAX_FORMAT, DVS_MAX_FIELDS(dvs_ptr));
	DVSDDEBUG("DVSINIT REQUEST " DVS_VER_FORMAT, DVS_VER_FIELDS(dvs_ptr));
	
	// fill command fields
	cmd_ptr->dvs_cmd 	= DVS_DVSINIT;
	cmd_ptr->dvs_snode 	= DVS_LOCALNODE;
	cmd_ptr->dvs_dnode 	= nodeid;
	cmd_ptr->dvs_lines 	= 1;
	cmd_ptr->dvs_rcode 	= 0;
	cmd_ptr->dvs_bmnodes = 0;
	cmd_ptr->dvs_paylen	= sizeof(dvs_usr_t);
	DVSDDEBUG("DVSINIT REQUEST dvscmd_ptr:" DVSCMD_FORMAT, DVSCMD_FIELDS(cmd_ptr));

	// fill the IO vector 
	rqst_iov[0].iov_base=cmd_ptr;
	rqst_iov[0].iov_len=sizeof(dvs_cmd_t);
	
	// fill the REQUEST message 
	rqst_ptr = &rqst_msg;
	rqst_ptr->msg_name		= &dvsd_addr;
	rqst_ptr->msg_namelen	= sizeof(dvsd_addr);
	rqst_ptr->msg_iov		= rqst_iov;
	rqst_ptr->msg_iovlen	= 1;
	rqst_ptr->msg_control	= (void *) out_aux_ptr;
	rqst_ptr->msg_controllen= sizeof(struct cmsghdr) + sizeof(dvs_usr_t));
	DVSDDEBUG("DVSINIT REQUEST rqst_ptr:" MSGHDR_FORMAT, MSGHDR_FIELDS(rqst_ptr));

	if ( (bytes = sendmsg(dvsd_sd, rqst_ptr,0))==-1) {
		DVSERR(errno);
	}
	
	// ------------- REPLY -----------------
	rply_iov[0].iov_base= udp_in_buf;
	rply_iov[0].iov_len	= sizeof(dvs_cmd_t);
	rply_ptr->msg_name	= &dvsd_addr;
	rply_ptr->msg_namelen= sizeof(dvsd_addr);
	rply_ptr->msg_iov	= rply_iov;
	rply_ptr->msg_iovlen	= 1;
	rply_ptr->msg_control= 0;
	rply_ptr->msg_controllen= 0;
	DVSDDEBUG("DVSINIT REPLY BEFORE rply_ptr:" MSGHDR_FORMAT, MSGHDR_FIELDS(rply_ptr));
	
	bytes =recvmsg(dvsd_sd,&rply_msg,0);
	if (bytes==-1) {
		DVSERR(-errno);
	} else if (rply_ptr->msg_flags&MSG_TRUNC) {
		DVSDDEBUG("datagram too large for buffer: truncated");
	}
	DVSDDEBUG("DVSINIT REPLY AFTER rply_ptr:" MSGHDR_FORMAT, MSGHDR_FIELDS(rply_ptr));

	DVSDDEBUG("DVSINIT REPLY: bytes=%d\n", bytes);
	if( bytes != sizeof(dvs_cmd_t) ) 
		ERROR_RETURN(EDVSMSGSIZE);
	cmd_ptr = (dvs_cmd_t *) udp_in_buf;
	DVSDDEBUG("DVSINIT REPLY cmd_ptr:" DVSCMD_FORMAT,DVSCMD_FIELDS(cmd_ptr));
	if( cmd_ptr->dvs_dvs_cmd != DVS_DVSINIT_ACK)
		ERROR_RETURN(EDVSBADVALUE);
	if( cmd_ptr->dvs_rcode != cmd_ptr->dvs_dnode) 
		ERROR_RETURN(cmd_ptr->dvs_rcode);
	return(cmd_ptr->dvs_rcode);
}

/*===========================================*
 *				cmd_dvsinfo				     *
 *===========================================*/
int cmd_dvsinfo(int argc, char *argv[]) 
{
	int nodeid, bytes;
	dvs_cmd_t *cmd_ptr;
	dvs_usr_t  *dvs_ptr;
	struct msghdr rqst_msg, *rqst_ptr;
	struct msghdr rply_msg, *rply_ptr;
	char *ptr;
	
	if( argc != 3){
		print_usage();
		ERROR_EXIT(EMOLINVAL);
	}
		
	nodeid = atoi(argv[2]);
	if (nodeid < 0 || nodeid > MAX_MEMBERS){
		fprintf(stderr,"Invalid nodeid (%d)\n", nodeid);
		ERROR_EXIT(EMOLBADNODEID)
	}
	
	cmd_ptr = (dvs_cmd_t *) udp_out_buf;
	
	//--------------- REQUEST -------------
	// fill command fields
	cmd_ptr->dvs_cmd 	= DVS_DVSINFO;
	cmd_ptr->dvs_snode 	= DVS_LOCALNODE;
	cmd_ptr->dvs_dnode 	= nodeid;
	cmd_ptr->dvs_lines 	= 0;
	cmd_ptr->dvs_rcode 	= 0;
	cmd_ptr->dvs_bmnodes= 0;
	cmd_ptr->dvs_paylen	= 0;
	DVSDDEBUG("DVSINFO REQUEST dvscmd_ptr:" DVSCMD_FORMAT, DVSCMD_FIELDS(cmd_ptr));
	
	rqst_iov[0].iov_base	= cmd_ptr;
	rqst_iov[0].iov_len		= sizeof(dvs_cmd_t);
	rqst_ptr = &rqst_msg;
	rqst_ptr->msg_name		= &dvsd_addr;
	rqst_ptr->msg_namelen	= sizeof(dvsd_addr);
	rqst_ptr->msg_iov		= rqst_iov;
	rqst_ptr->msg_iovlen	= 1;
	rqst_ptr->msg_control	= 0;
	rqst_ptr->msg_controllen= 0;
	DVSDDEBUG("DVSINFO REQUEST rqst_ptr:" MSGHDR_FORMAT, MSGHDR_FIELDS(rqst_ptr));
	if ( (bytes = sendmsg(dvsd_sd,&rqst_msg,0))==-1) {
		DVSERR(errno);
	}
	
	// ------------- REPLY -----------------
	in_aux_ptr = (struct cmsghdr *) udp_in_buf;
	rply_iov[0].iov_base	= cmd_ptr;
	rply_iov[0].iov_len		= sizeof(dvs_cmd_t);
	rply_ptr = &rply_msg;
	rply_ptr->msg_name		= &dvsd_addr;
	rply_ptr->msg_namelen	= sizeof(dvsd_addr);
	rply_ptr->msg_iov		= rply_iov;
	rply_ptr->msg_iovlen	= 1;
	rply_ptr->msg_control	= in_aux_ptr;
	rply_ptr->msg_controllen= sizeof(struct cmsghdr) + sizeof(dvs_usr_t));
	DVSDDEBUG("DVSINFO BEFORE REPLY rply_ptr:" MSGHDR_FORMAT, MSGHDR_FIELDS(rply_ptr));
		
	bytes =recvmsg(dvsd_sd,&rply_msg,0);
	if (bytes==-1) {
		DVSERR(-errno);
	} else if (rply_ptr->msg_flags&MSG_TRUNC) {
		DVSDDEBUG("datagram too large for buffer: truncated");
	}
	DVSDDEBUG("DVSINFO REPLY cmd_ptr:" DVSCMD_FORMAT,DVSCMD_FIELDS(cmd_ptr));
	DVSDDEBUG("DVSINFO REPLY: bytes=%d\n", bytes);
	if( cmd_ptr->dvs_dvs_cmd != DVS_DVSINFO_ACK)
		ERROR_RETURN(EDVSBADVALUE);
	if( cmd_ptr->dvs_rcode != cmd_ptr->dvs_dnode) 
		ERROR_RETURN(cmd_ptr->dvs_rcode);

	DVSDDEBUG("DVSINFO AFTER REPLY rply_ptr:" MSGHDR_FORMAT, MSGHDR_FIELDS(rply_ptr));
	
	dvs_ptr = (dvs_usr_t  *) CMSG_LEN(sizeof(dvs_usr_t));
	printf("DVSINFO REPLY " DVS_USR_FORMAT, DVS_USR_FIELDS(dvs_ptr));
	printf("DVSINFO REPLY " DVS_MAX_FORMAT, DVS_MAX_FIELDS(dvs_ptr));
	printf("DVSINFO REPLY " DVS_VER_FORMAT, DVS_VER_FIELDS(dvs_ptr));
	
	return(OK);
}

/*===========================================*
 *				cmd_dvsend				     *
 *===========================================*/
int cmd_dvsend(int argc, char *argv[]) 
{
	int nodeid, bytes;
	dvs_cmd_t *cmd_ptr;
	struct msghdr rqst_msg, *rqst_ptr;
	struct msghdr rply_msg, *rply_ptr;
	
	if( argc != 3){
		print_usage();
		ERROR_EXIT(EMOLINVAL);
	}
		
	nodeid = atoi(argv[2]);
	if (nodeid < 0 || nodeid > MAX_MEMBERS){
		fprintf(stderr,"Invalid nodeid (%d)\n", nodeid);
		ERROR_EXIT(EMOLBADNODEID)
	}
	
	cmd_ptr = (dvs_cmd_t *) udp_out_buf;
	
	//------------------ REQUEST -------------
	// fill command fields
	cmd_ptr->dvs_cmd 	= DVS_DVSEND;
	cmd_ptr->dvs_snode 	= DVS_LOCALNODE;
	cmd_ptr->dvs_dnode 	= nodeid;
	cmd_ptr->dvs_lines 	= 0;
	cmd_ptr->dvs_rcode 	= 0;
	cmd_ptr->dvs_bmnodes= 0;
	cmd_ptr->dvs_paylen	= 0;
	DVSDDEBUG("DVSEND REQUEST dvscmd_ptr:" DVSCMD_FORMAT, DVSCMD_FIELDS(cmd_ptr));
		
	rqst_iov[0].iov_base=cmd_ptr;
	rqst_iov[0].iov_len	=sizeof(dvs_cmd_t);
	rqst_ptr = &rqst_msg;
	rqst_ptr->msg_name		= &dvsd_addr;
	rqst_ptr->msg_namelen	= sizeof(dvsd_addr);
	rqst_ptr->msg_iov		= rqst_iov;
	rqst_ptr->msg_iovlen	= 1;
	rqst_ptr->msg_control	= 0;
	rqst_ptr->msg_controllen= 0;
	DVSDDEBUG("DVSEND REQUEST rqst_ptr:" MSGHDR_FORMAT, MSGHDR_FIELDS(rqst_ptr));

	if ( (bytes = sendmsg(dvsd_sd,&rqst_msg,0))==-1) {
		DVSERR(errno);
	}
	
	// ------------- REPLY -----------------
	rply_iov[0].iov_base=udp_in_buf;
	rply_iov[0].iov_len=sizeof(dvs_cmd_t);
	rply_ptr = &rply_msg;
	rply_ptr->msg_name		= &dvsd_addr;
	rply_ptr->msg_namelen	= sizeof(dvsd_addr);
	rply_ptr->msg_iov		= rply_iov;
	rply_ptr->msg_iovlen	= 1;
	rply_ptr->msg_control	= 0;
	rply_ptr->msg_controllen= 0;
	DVSDDEBUG("DVSEND BEFORE REPLY rply_ptr:" MSGHDR_FORMAT, MSGHDR_FIELDS(rply_ptr));

	bytes =recvmsg(dvsd_sd,&rply_msg,0);
	if (bytes==-1) {
		DVSERR(-errno);
	} else if (rply_ptr->msg_flags&MSG_TRUNC) {
		DVSDDEBUG("datagram too large for buffer: truncated");
	}
	DVSDDEBUG("DVSEND AFTER REPLY rply_ptr:" MSGHDR_FORMAT, MSGHDR_FIELDS(rply_ptr));

	cmd_ptr = (dvs_cmd_t *) udp_in_buf;
   	DVSDDEBUG("DVSEND cmd_ptr:" DVSCMD_FORMAT,DVSCMD_FIELDS(cmd_ptr));
	if( cmd_ptr->dvs_dvs_cmd != DVS_DVSEND_ACK)
		ERROR_RETURN(EDVSBADVALUE);
	if( cmd_ptr->dvs_rcode != cmd_ptr->dvs_dnode) 
		ERROR_RETURN(cmd_ptr->dvs_rcode);
	return(cmd_ptr->dvs_rcode);
}

/*===========================================================================*
 *				udp_connect				     *
 *===========================================================================*/
int udp_connect(void){
	
    int port_no, rcode, i;
    char rmt_ipaddr[INET_ADDRSTRLEN+1];

    port_no = DVSD_PORT;
	DVSDDEBUG("DVSCMD: connect to DVSD server running at UDP port=%d\n", port_no);    

    memset(&dvsd_addr, 0, sizeof(dvsd_addr)); 
    dvsd_addr.sin_family = AF_INET;  
    dvsd_addr.sin_port = htons(port_no);  
    dvsd_addr.sin_addr.s_addr = INADDR_ANY; 

    dvsd_host = gethostbyname("localhost");
	if( dvsd_host == NULL) ERROR_EXIT(-errno);
	for( i =0; dvsd_host->h_addr_list[i] != NULL; i++) {
		DVSDDEBUG("DVSCMD: remote host address %i: %s\n", 
			i, inet_ntoa( *( struct in_addr*)(dvsd_host->h_addr_list[i])));
	}

    if((inet_pton(AF_INET,inet_ntoa( *( struct in_addr*)(dvsd_host->h_addr_list[0])), (struct sockaddr*) &dvsd_addr.sin_addr)) <= 0)
    	ERROR_RETURN(-errno);

    inet_ntop(AF_INET, (struct sockaddr*) &dvsd_addr.sin_addr, rmt_ipaddr, INET_ADDRSTRLEN);
	DVSDDEBUG("DVSCMD: for node %s running at  IP=%s\n", px.px_name, rmt_ipaddr);    

	    // Creating socket file descriptor 
    if ( (dvsd_sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        ERROR_RETURN(EXIT_FAILURE); 
    } 

	posix_memalign( (void**) &cmd_ptr, getpagesize(), sizeof(dvs_cmd_t) );
	if( cmd_ptr == NULL) return (EMOLNOMEM);
	
	posix_memalign( (void**) &udp_in_buf, getpagesize(), MAX_MESSLEN );
	if( udp_in_buf == NULL) return (EMOLNOMEM);

	posix_memalign( (void**) &udp_out_buf, getpagesize(), MAX_MESSLEN );
	if( udp_out_buf == NULL) return (EMOLNOMEM);
	
	return(OK);
}

/*===========================================================================*
 *				dvscmd_init				     *
 *===========================================================================*/
int dvscmd_init(void)
{
	int  dvscmd_pid, i, rcode;
	
	dvscmd_pid = getpid();
   	DVSDDEBUG( "dvscmd_pid=%d\n",  dvscmd_pid);
	
  	DVSDDEBUG( "Initialize the call vector to a safe default handler.\n");
  	for (i=0; i<DVS_MAXCMDS; i++) {
		DVSDDEBUG("Initilizing vector %d on address=%p\n",i, cmd_unused);
      	call_vec[i] = cmd_unused;
  	}

    map(DVS_DVSINIT, 	cmd_dvsinit); 
	DVSDDEBUG("Initilizing vector %d on address=%p\n",DVS_DVSINIT, cmd_dvsinit);
    map(DVS_DVSINFO, 	cmd_dvsinfo);		
	DVSDDEBUG("Initilizing vector %d on address=%p\n",DVS_DVSINFO, cmd_dvsinfo);
	map(DVS_DVSEND, 	cmd_dvsend);		
	DVSDDEBUG("Initilizing vector %d on address=%p\n",DVS_DVSEND, cmd_dvsend);
	
	return(OK);
}

/****************************************************************************************/
/*			DVSCMD 						*/
/****************************************************************************************/
int  main ( int argc, char *argv[] )
{
	int rcode, cmd_nr, result, bytes, cmd_argc, i;
	char *cmd_ptr;
	dvs_cmd_t *incmd_ptr, *outcmd_ptr;
	
	if ( argc < 2 ) {
		print_usage();
 	    exit(1);
    }

	cmd_ptr = argv[1];
	cmd_argc = argc - 1;

	DVSDDEBUG("cmd_ptr=>%s<\n", cmd_ptr);

	// get the command number
	for ( cmd_nr = 1; cmd_nr < DVS_MAXCMDS; cmd_nr++){
		DVSDDEBUG("cmd_str[%d]=>%s< cmd_ptr=>%s<\n", cmd_nr, cmd_str[cmd_nr], cmd_ptr);
		if ( strcasecmp (cmd_str[cmd_nr], cmd_ptr) )
			continue;
		break;
	}
	
	// invalid command number
	if ( cmd_nr >= DVS_MAXCMDS){
		print_usage();
 	    exit(1);
    }
	
	DVSDDEBUG("cmd_ptr=%s cmd_argc=%d cmd_nr=%d\n", cmd_ptr, cmd_argc, cmd_nr);
	for( i = 0; i < cmd_argc; i++){
		DVSDDEBUG("\t cmd_arg[%d]=%s\n", i, argv[i+1]);
	} 
	
	rcode = udp_connect();
	if( rcode) {	
		DVSDDEBUG("udp_connect rcode=%d\n",rcode);
		ERROR_EXIT(rcode);
	}

	dvscmd_init();
	
   	result = (*call_vec[cmd_nr])(argc, argv);		/* handle the call*/
	
	if(result) ERROR_RETURN(result);
	return(OK);
 }





