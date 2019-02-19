/****************************************************************/
/* 				DVSCMD				*/
/* COMMAND LINE INTERFACE */
/****************************************************************/

#define _DVSD
#define _DVSCMD
#include "dvsd.h"
#include "glo.h"

//#define OPTION_WRITE 1
//#define OPTION_READ  1

int (*call_vec[DVS_MAXCMDS])(int argc, char *argv[]);
#define map(cmd_nr, handler) call_vec[(cmd_nr)] = (handler)

struct hostent *dvsd_host;

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

dc_usr_t dc_lcl = {
	.dc_dcid = 0,
	.dc_flags = 0,
    .dc_nr_procs = NR_PROCS,
	.dc_nr_tasks = NR_TASKS,
	.dc_nr_sysprocs = NR_SYS_PROCS,
	.dc_nr_nodes = NR_NODES,
	.dc_warn2proc = NONE,
	.dc_warnmsg = KERNEL,
	.dc_name = "DC0"
};


print_usage(void){
	fprintf(stderr,"Usage: dvscmd <nodeid> <dvscmd> [<args ...>]\n");
	fprintf(stderr,"DVS Commands\n");

	fprintf(stderr,"DVSINIT	[-C nr_dcs] [-N nr_nodes] [-P nr_procs] [-T nr_tasks]"
						" [-B max_copybuf] [-L max_copylen] \n" 
						" [-S nr_sysprocs] [-D dbglvl]\n");
	fprintf(stderr,"DVSEND\n");

	fprintf(stderr,"DVSINFO\n");
	fprintf(stderr,"DCINFO <dcid>\n");
	fprintf(stderr,"NODEINFO <nodeid> \n");  
	fprintf(stderr,"PROXYINFO <pxid> \n");  
	fprintf(stderr,"PROCINFO <dcid> <p_nr> \n");  
	 	  
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

int cmd_unused(int argc, char *argv[])
{
	ERROR_PRINT(EDVSNOSYS);
}

/*===========================================*
 *				cmd_dvsinit				     *
 * dvscmd <nodeid> DVSINIT <args.....>		 *
 *===========================================*/
int cmd_dvsinit(int argc, char *argv[]) 
{
	int nodeid, bytes, c;
	char *ptr;
	dvs_usr_t *dvs_ptr;
	dvs_cmd_t *cmd_ptr;
    int n, len, total, maxdata, received = 0;
	
	USRDEBUG("DVSINIT argc=%d\n", argc);

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

	nodeid = atoi(argv[1]);
	if (nodeid < 0 || nodeid >= MAX_MEMBERS){
		fprintf(stderr,"Invalid nodeid (%d)\n", nodeid);
		ERROR_EXIT(EDVSBADNODEID)
	}
	USRDEBUG("DVSINIT nodeid=%d\n", nodeid);

	// ------------- REQUEST  -----------------
	// the command is passed in the request IOV
	// the argument/data is passed in the ancillary data 
	
	// set the pointer to payload: dvs_usr_t 
	cmd_ptr = (dvs_cmd_t *) tcp_out_buf;
	ptr = (char *) cmd_ptr;
	ptr += sizeof(dvs_cmd_t);
	dvs_ptr = (dvs_usr_t*) ptr; 
	
	// copy from local DVS user struct to msgq message payload
	memcpy((void *) dvs_ptr, (void *) &dvs_lcl, sizeof(dvs_usr_t));
	USRDEBUG("DVSINIT REQUEST " DVS_USR_FORMAT, DVS_USR_FIELDS(dvs_ptr));
	USRDEBUG("DVSINIT REQUEST " DVS_MAX_FORMAT, DVS_MAX_FIELDS(dvs_ptr));
	USRDEBUG("DVSINIT REQUEST " DVS_VER_FORMAT, DVS_VER_FIELDS(dvs_ptr));
	
	// fill command fields
	cmd_ptr->dvs_cmd 	= DVS_DVSINIT;
	cmd_ptr->dvs_snode 	= DVS_LOCALNODE;
	cmd_ptr->dvs_dnode 	= nodeid;
	cmd_ptr->dvs_arg1 	= nodeid;
	cmd_ptr->dvs_arg2 	= 0;
	cmd_ptr->dvs_lines 	= 1;
	cmd_ptr->dvs_rcode 	= 0;
	cmd_ptr->dvs_bmnodes = 0;
	cmd_ptr->dvs_paylen	= sizeof(dvs_usr_t);
	USRDEBUG("DVSINIT REQUEST cmd_ptr:" DVSRQST_FORMAT, DVSRQST_FIELDS(cmd_ptr));
#ifdef OPTION_WRITE 
	bytes = sizeof(dvs_cmd_t)+ cmd_ptr->dvs_paylen;
	if ( (bytes = write(dvsd_sd, tcp_out_buf, bytes)) < 0) {
		ERROR_RETURN(errno);
	}
	USRDEBUG("DVSINIT REQUEST: bytes=%d\n", bytes);
#else // OPTION_WRITE 
	if ( (bytes = send(dvsd_sd, tcp_out_buf, 
				sizeof(dvs_cmd_t)+ cmd_ptr->dvs_paylen, 0)) < 0) {
		ERROR_RETURN(errno);
	}
	USRDEBUG("DVSINIT REQUEST: bytes=%d\n", bytes);
#endif // OPTION_WRITE 

	// ------------- REPLY -----------------
#ifdef OPTION_READ
	if( (bytes = read(dvsd_sd, tcp_in_buf, MAX_MESSLEN)) < 0) {
		ERROR_RETURN(errno);
	}
	USRDEBUG("DVSINIT REPLY: bytes=%d\n", bytes);
#else // OPTION_READ
	if( (bytes = recv(dvsd_sd, tcp_in_buf, MAX_MESSLEN, 0 )) < 0) {
		ERROR_RETURN(errno);
	}
	USRDEBUG("DVSINIT REPLY: bytes=%d\n", bytes);
#endif // OPTION_READ

	if( bytes != sizeof(dvs_cmd_t) ) 
		ERROR_RETURN(EDVSMSGSIZE);
	cmd_ptr = (dvs_cmd_t *) tcp_in_buf;
	USRDEBUG("DVSINIT REPLY cmd_ptr:" DVSRPLY_FORMAT,DVSRPLY_FIELDS(cmd_ptr));
	if( cmd_ptr->dvs_cmd != DVS_DVSINIT_ACK)
		ERROR_RETURN(EDVSBADVALUE);
	if( cmd_ptr->dvs_rcode != nodeid) 
		ERROR_RETURN(cmd_ptr->dvs_rcode);
	return(cmd_ptr->dvs_rcode);
}

/*===========================================*
 *				cmd_dvsend			*
 * dvscmd <nodeid> DVSEND 				*
 *===========================================*/
int cmd_dvsend(int argc, char *argv[]) 
{
	int nodeid, bytes;
	dvs_cmd_t *cmd_ptr;
	
	if( argc != 3){
		print_usage();
		ERROR_EXIT(EDVSINVAL);
	}
		
	nodeid = atoi(argv[1]);
	if (nodeid < 0 || nodeid >= MAX_MEMBERS){
		fprintf(stderr,"Invalid nodeid (%d)\n", nodeid);
		ERROR_EXIT(EDVSBADNODEID)
	}
	
	cmd_ptr = (dvs_cmd_t *) tcp_out_buf;
	
	//------------------ REQUEST -------------
	// fill command fields
	cmd_ptr->dvs_cmd 	= DVS_DVSEND;
	cmd_ptr->dvs_snode 	= DVS_LOCALNODE;
	cmd_ptr->dvs_dnode 	= nodeid;
	cmd_ptr->dvs_arg1 	= nodeid;
	cmd_ptr->dvs_arg2 	= 0;
	cmd_ptr->dvs_lines 	= 0;
	cmd_ptr->dvs_rcode 	= 0;
	cmd_ptr->dvs_bmnodes= 0;
	cmd_ptr->dvs_paylen	= 0;
	USRDEBUG("DVSEND REQUEST dvscmd_ptr:" DVSRQST_FORMAT, DVSRQST_FIELDS(cmd_ptr));
	
	if ( (bytes = send(dvsd_sd, tcp_out_buf, 
				sizeof(dvs_cmd_t), 0)) < 0) {
		ERROR_RETURN(errno);
	}
	USRDEBUG("DVSEND REQUEST: bytes=%d\n", bytes);

	// ------------- REPLY -----------------
	if( (bytes = recv(dvsd_sd, tcp_in_buf, MAX_MESSLEN, 0 )) < 0) {
		ERROR_RETURN(errno);
	}
	USRDEBUG("DVSEND REPLY: bytes=%d\n", bytes);
	
	cmd_ptr = (dvs_cmd_t *) tcp_in_buf;
   	USRDEBUG("DVSEND cmd_ptr:" DVSRPLY_FORMAT,DVSRPLY_FIELDS(cmd_ptr));
	if( cmd_ptr->dvs_cmd != DVS_DVSEND_ACK)
		ERROR_RETURN(EDVSBADVALUE);
	if( cmd_ptr->dvs_rcode != OK) 
		ERROR_RETURN(cmd_ptr->dvs_rcode);
	return(cmd_ptr->dvs_rcode);
}

/*===========================================*
 *				cmd_dvsinfo		        *
 * dvscmd <nodeid> DVSINFO				 *
 *===========================================*/
int cmd_dvsinfo(int argc, char *argv[]) 
{
	int nodeid, bytes;
	dvs_cmd_t *cmd_ptr;
	dvs_usr_t  *dvs_ptr;
	char *ptr;
	
	if( argc != 3){
		print_usage();
		ERROR_EXIT(EDVSINVAL);
	}
		
	nodeid = atoi(argv[1]);
	if (nodeid < 0 || nodeid >= MAX_MEMBERS){
		fprintf(stderr,"Invalid nodeid (%d)\n", nodeid);
		ERROR_EXIT(EDVSBADNODEID)
	}
	
	cmd_ptr = (dvs_cmd_t *) tcp_out_buf;
	
	//--------------- REQUEST -------------
	// fill command fields
	cmd_ptr->dvs_cmd 	= DVS_DVSINFO;
	cmd_ptr->dvs_snode 	= DVS_LOCALNODE;
	cmd_ptr->dvs_dnode 	= nodeid;
	cmd_ptr->dvs_arg1	= nodeid;
	cmd_ptr->dvs_arg2 	= 0;
	cmd_ptr->dvs_lines 	= 0;
	cmd_ptr->dvs_rcode 	= 0;
	cmd_ptr->dvs_bmnodes= 0;
	cmd_ptr->dvs_paylen	= 0;
	USRDEBUG("DVSINFO REQUEST dvscmd_ptr:" DVSRQST_FORMAT, DVSRQST_FIELDS(cmd_ptr));

#ifdef OPTION_WRITE 
	bytes = sizeof(dvs_cmd_t)+ cmd_ptr->dvs_paylen;
	if ( (bytes = write(dvsd_sd, tcp_out_buf, bytes)) < 0) {
		ERROR_RETURN(errno);
	}
	USRDEBUG("DVSINFO REQUEST: bytes=%d\n", bytes);
#else // OPTION_WRITE 
	if ( (bytes = send(dvsd_sd, tcp_out_buf, 
				sizeof(dvs_cmd_t)+ cmd_ptr->dvs_paylen, 0)) < 0) {
		ERROR_RETURN(errno);
	}
	USRDEBUG("DVSINFO REQUEST: bytes=%d\n", bytes);
#endif // OPTION_WRITE 

	// ------------- REPLY -----------------
#ifdef OPTION_READ
	if( (bytes = read(dvsd_sd, tcp_in_buf, MAX_MESSLEN)) < 0) {
		ERROR_RETURN(errno);
	}
	USRDEBUG("DVSINFO REPLY: bytes=%d\n", bytes);
#else // OPTION_READ
	if( (bytes = recv(dvsd_sd, tcp_in_buf, MAX_MESSLEN, 0 )) < 0) {
		ERROR_RETURN(errno);
	}
	USRDEBUG("DVSINFO REPLY: bytes=%d\n", bytes);
#endif // OPTION_READ

	cmd_ptr = (dvs_cmd_t *) tcp_in_buf;
	USRDEBUG("DVSINFO REPLY cmd_ptr:" DVSRPLY_FORMAT,DVSRPLY_FIELDS(cmd_ptr));
	if( cmd_ptr->dvs_cmd != DVS_DVSINFO_ACK)
		ERROR_RETURN(EDVSBADVALUE);
	if( cmd_ptr->dvs_rcode != OK) 
		ERROR_RETURN(cmd_ptr->dvs_rcode);
	if( bytes != ((sizeof(dvs_cmd_t) + sizeof(dvs_usr_t))))
		ERROR_RETURN(EDVSMSGSIZE);
		
	ptr = (char *) cmd_ptr;
	ptr += sizeof(dvs_cmd_t);
	dvs_ptr = (dvs_usr_t  *) ptr;

	if (nodeid != cmd_ptr->dvs_dnode){
		fprintf(stderr,"Invalid dvs_dnode (%d)\n", cmd_ptr->dvs_dnode);
		ERROR_EXIT(EDVSBADNODEID)
	}
	
	printf(DVS_USR_FORMAT, DVS_USR_FIELDS(dvs_ptr));
	printf(DVS_MAX_FORMAT, DVS_MAX_FIELDS(dvs_ptr));
	printf(DVS_VER_FORMAT, DVS_VER_FIELDS(dvs_ptr));
	
	return(OK);
}

/*===========================================*
 *				cmd_dcinfo				     *
 * dvscmd <nodeid> DCINFO <dcid>			 *
 *===========================================*/
int cmd_dcinfo(int argc, char *argv[]) 
{
	int nodeid, dcid, bytes;
	dvs_cmd_t *cmd_ptr;
	dc_usr_t  *dc_ptr;
	char *ptr;
	
	USRDEBUG("argc=%d\n",argc);

	if( argc != 4){
		print_usage();
		ERROR_EXIT(EDVSINVAL);
	}
		
	nodeid = atoi(argv[1]);
	if (nodeid < 0 || nodeid >= MAX_MEMBERS){
		fprintf(stderr,"Invalid nodeid (%d)\n", nodeid);
		ERROR_EXIT(EDVSBADNODEID)
	}

	dcid = atoi(argv[3]);
	if (dcid < 0 || dcid >= NR_DCS){
		fprintf(stderr,"Invalid dcid (%d)\n", dcid);
		ERROR_EXIT(EDVSBADDCID)
	}
		
	cmd_ptr = (dvs_cmd_t *) tcp_out_buf;
	
	//--------------- REQUEST -------------
	// fill command fields
	cmd_ptr->dvs_cmd 	= DVS_DCINFO;
	cmd_ptr->dvs_snode 	= DVS_LOCALNODE;
	cmd_ptr->dvs_dnode 	= nodeid;
	cmd_ptr->dvs_arg1 	= dcid;
	cmd_ptr->dvs_arg2 	= 0;
	cmd_ptr->dvs_lines 	= 0;
	cmd_ptr->dvs_rcode 	= 0;
	cmd_ptr->dvs_bmnodes= 0;
	cmd_ptr->dvs_paylen	= 0;
	USRDEBUG("DCINFO REQUEST dvscmd_ptr:" DVSRQST_FORMAT, DVSRQST_FIELDS(cmd_ptr));

	if ( (bytes = send(dvsd_sd, tcp_out_buf, 
				sizeof(dvs_cmd_t)+cmd_ptr->dvs_paylen, 0)) < 0) {
		ERROR_RETURN(errno);
	}
	USRDEBUG("DCINFO REQUEST: bytes=%d\n", bytes);

	// ------------- REPLY -----------------
	if( (bytes = recv(dvsd_sd, tcp_in_buf, MAX_MESSLEN, 0 )) < 0) {
		ERROR_RETURN(errno);
	}
	USRDEBUG("DCINFO REPLY: bytes=%d\n", bytes);

	cmd_ptr = (dvs_cmd_t *) tcp_in_buf;
	USRDEBUG("DCINFO REPLY cmd_ptr:" DVSRPLY_FORMAT,DVSRPLY_FIELDS(cmd_ptr));
	if( cmd_ptr->dvs_cmd != DVS_DCINFO_ACK)
		ERROR_RETURN(EDVSBADVALUE);
	if( cmd_ptr->dvs_rcode != OK) 
		ERROR_RETURN(cmd_ptr->dvs_rcode);
	if( bytes != ((sizeof(dvs_cmd_t) + sizeof(dc_usr_t))))
		ERROR_RETURN(EDVSMSGSIZE);
		
	ptr = (char *) cmd_ptr;
	ptr += sizeof(dvs_cmd_t);
    dc_ptr = (dc_usr_t*) ptr;
	
	if (dcid != dc_ptr->dc_dcid){
		fprintf(stderr,"Invalid dc_dcid (%d)\n", dc_ptr->dc_dcid);
		ERROR_EXIT(EDVSBADDCID);
	}
	
	printf(DC_USR1_FORMAT, DC_USR1_FIELDS(dc_ptr));
	printf(DC_USR2_FORMAT, DC_USR2_FIELDS(dc_ptr));
	
	return(OK);
}

/*===========================================*
 *				cmd_nodeinfo			     *
 * dvscmd <nodeid> NODEINFO <dcnode>		 *
 *===========================================*/
int cmd_nodeinfo(int argc, char *argv[]) 
{
	int nodeid, bytes, dcnode;
	dvs_cmd_t *cmd_ptr;
	node_usr_t *node_ptr;
	char *ptr;
	
	if( argc != 4){
		print_usage();
		ERROR_EXIT(EDVSINVAL);
	}
		
	nodeid = atoi(argv[1]);
	if (nodeid < 0 || nodeid >= MAX_MEMBERS){
		fprintf(stderr,"Invalid nodeid (%d)\n", nodeid);
		ERROR_EXIT(EDVSBADNODEID)
	}

	dcnode = atoi(argv[3]);
	if (dcnode < 0 || dcnode >= NR_NODES){
		fprintf(stderr,"Invalid DC nodeid (%d)\n", dcnode);
		ERROR_EXIT(EDVSBADNODEID)
	}
	
	cmd_ptr = (dvs_cmd_t *) tcp_out_buf;
	
	//------------------ REQUEST -------------
	// fill command fields
	cmd_ptr->dvs_cmd 	= DVS_NODEINFO;
	cmd_ptr->dvs_snode 	= DVS_LOCALNODE;
	cmd_ptr->dvs_dnode 	= nodeid;
	cmd_ptr->dvs_arg1 	= dcnode;
	cmd_ptr->dvs_arg2 	= 0;
	cmd_ptr->dvs_lines 	= 0;
	cmd_ptr->dvs_rcode 	= 0;
	cmd_ptr->dvs_bmnodes= 0;
	cmd_ptr->dvs_paylen	= 0;
	USRDEBUG("NODEINFO REQUEST dvscmd_ptr:" DVSRQST_FORMAT, DVSRQST_FIELDS(cmd_ptr));
	
	if ( (bytes = send(dvsd_sd, tcp_out_buf, 
				sizeof(dvs_cmd_t) + cmd_ptr->dvs_paylen, 0)) < 0) {
		ERROR_RETURN(errno);
	}
	USRDEBUG("NODEINFO REQUEST: bytes=%d\n", bytes);

	// ------------- REPLY -----------------
	if( (bytes = recv(dvsd_sd, tcp_in_buf, MAX_MESSLEN, 0 )) < 0) {
		ERROR_RETURN(errno);
	}
	USRDEBUG("NODEINFO REPLY: bytes=%d\n", bytes);
	
	cmd_ptr = (dvs_cmd_t *) tcp_in_buf;
   	USRDEBUG("NODEINFO cmd_ptr:" DVSRPLY_FORMAT,DVSRPLY_FIELDS(cmd_ptr));
	if( cmd_ptr->dvs_cmd != DVS_NODEINFO_ACK)
		ERROR_RETURN(EDVSBADVALUE);
	if( cmd_ptr->dvs_rcode != OK) 
		ERROR_RETURN(cmd_ptr->dvs_rcode);
	
	ptr = (char *) cmd_ptr;
	ptr += sizeof(dvs_cmd_t);
	node_ptr = (node_usr_t *) ptr;
	
	if (dcnode != node_ptr->n_nodeid){
		fprintf(stderr,"Invalid n_nodeid (%d)\n",  node_ptr->n_nodeid);
		ERROR_EXIT(EDVSBADDCID);
	}	
	
	printf( NODE_USR_FORMAT, NODE_USR_FIELDS(node_ptr));
	printf( NODE_TIME_FORMAT, NODE_TIME_FIELDS(node_ptr));

	return(cmd_ptr->dvs_rcode);
}

/*===========================================*
 *				cmd_proxyinfo			     *
 * dvscmd <nodeid> PROXYINFO <pxid>		 *
 *===========================================*/
int cmd_proxyinfo(int argc, char *argv[]) 
{
	int nodeid, bytes, pxid;
	dvs_cmd_t *cmd_ptr;
	proc_usr_t *sproxy_ptr;
	proc_usr_t *rproxy_ptr;
	proxies_usr_t *px_ptr;
	char *ptr;
	
	if( argc != 4){
		print_usage();
		ERROR_EXIT(EDVSINVAL);
	}
		
	nodeid = atoi(argv[1]);
	if (nodeid < 0 || nodeid >= MAX_MEMBERS){
		fprintf(stderr,"Invalid nodeid (%d)\n", nodeid);
		ERROR_EXIT(EDVSBADNODEID);
	}

	pxid = atoi(argv[3]);
	if (pxid < 0 || pxid >= NR_NODES){
		fprintf(stderr,"Invalid Proxy ID (%d)\n", pxid);
		ERROR_EXIT(EDVSPROXYID);
	}
	
	cmd_ptr = (dvs_cmd_t *) tcp_out_buf;
	
	//------------------ REQUEST -------------
	// fill command fields
	cmd_ptr->dvs_cmd 	= DVS_PROXYINFO;
	cmd_ptr->dvs_snode 	= DVS_LOCALNODE;
	cmd_ptr->dvs_dnode 	= nodeid;
	cmd_ptr->dvs_arg1 	= pxid;
	cmd_ptr->dvs_arg2 	= 0;
	cmd_ptr->dvs_lines 	= 0;
	cmd_ptr->dvs_rcode 	= 0;
	cmd_ptr->dvs_bmnodes= 0;
	cmd_ptr->dvs_paylen	= 0;
	USRDEBUG("PROXYINFO REQUEST dvscmd_ptr:" DVSRQST_FORMAT, DVSRQST_FIELDS(cmd_ptr));
	
	if ( (bytes = send(dvsd_sd, tcp_out_buf, 
				sizeof(dvs_cmd_t) + cmd_ptr->dvs_paylen	, 0)) < 0) {
		ERROR_RETURN(errno);
	}
	USRDEBUG("PROXYINFO REQUEST: bytes=%d\n", bytes);

	// ------------- REPLY -----------------
	if( (bytes = recv(dvsd_sd, tcp_in_buf, MAX_MESSLEN, 0 )) < 0) {
		ERROR_RETURN(errno);
	}
	USRDEBUG("PROXYINFO REPLY: bytes=%d\n", bytes);
	
	cmd_ptr = (dvs_cmd_t *) tcp_in_buf;
   	USRDEBUG("PROXYINFO cmd_ptr:" DVSRPLY_FORMAT,DVSRPLY_FIELDS(cmd_ptr));
	if( cmd_ptr->dvs_cmd != DVS_PROXYINFO_ACK)
		ERROR_RETURN(EDVSBADVALUE);
	if( cmd_ptr->dvs_rcode != OK) 
		ERROR_RETURN(cmd_ptr->dvs_rcode);
	if( cmd_ptr->dvs_lines != 3) // 1 for proxy, 2 for proccess descriptors
		ERROR_RETURN(cmd_ptr->dvs_rcode);

	// FIRST: get proxy descriptor 	
	ptr = (char *) cmd_ptr;
	ptr += sizeof(dvs_cmd_t);
	px_ptr = (proxies_usr_t *) ptr;
#ifdef PROXY_ATTR
	if (pxid != px_ptr->px_id){
		fprintf(stderr,"Invalid px_id (%d)\n", px_ptr->px_id);
		ERROR_EXIT(EDVSPROXYID);
	}	
	printf(PX_USR_FORMAT, PX_USR_FIELDS(px_ptr));
#endif // PROXY_ATTR
	
	// SECOND: Get SPROXY process descriptor
	ptr += sizeof(proxies_usr_t);
	sproxy_ptr = (proc_usr_t *) ptr;
	printf( PROC_USR_FORMAT, PROC_USR_FIELDS(sproxy_ptr));
	printf( PROC_WAIT_FORMAT, PROC_WAIT_FIELDS(sproxy_ptr));
	printf( PROC_COUNT_FORMAT, PROC_COUNT_FIELDS(sproxy_ptr));
	
	// THIRD: Get RPROXY process descriptor
	ptr += sizeof(proc_usr_t);
	rproxy_ptr = (proc_usr_t *) ptr;
	printf( PROC_USR_FORMAT, PROC_USR_FIELDS(rproxy_ptr));
	printf( PROC_WAIT_FORMAT, PROC_WAIT_FIELDS(rproxy_ptr));
	printf( PROC_COUNT_FORMAT, PROC_COUNT_FIELDS(rproxy_ptr));

	return(cmd_ptr->dvs_rcode);
}

/*===========================================*
 *				cmd_procinfo		     *
 * dvscmd <nodeid> PROCINFO <dcid> <pnr>	 *
 *===========================================*/
int cmd_procinfo(int argc, char *argv[]) 
{
	int nodeid, bytes, dcid, pnr;
	dvs_cmd_t *cmd_ptr;
	proc_usr_t *proc_ptr;
	char *ptr;
	
	if( argc != 5){
		print_usage();
		ERROR_EXIT(EDVSINVAL);
	}
		
	nodeid = atoi(argv[1]);
	if (nodeid < 0 || nodeid >= MAX_MEMBERS){
		fprintf(stderr,"Invalid nodeid (%d)\n", nodeid);
		ERROR_EXIT(EDVSBADNODEID);
	}

	dcid = atoi(argv[3]);
	if (dcid < 0 || dcid >= NR_DCS){
		fprintf(stderr,"Invalid dcid (%d)\n", dcid);
		ERROR_EXIT(EDVSBADDCID)
	}

	pnr = atoi(argv[4]);
	if (pnr < (-NR_TASKS) || pnr >= NR_PROCS){
		fprintf(stderr,"Invalid pnr (%d)\n", pnr);
		ERROR_EXIT(EDVSBADPROC)
	}

	cmd_ptr = (dvs_cmd_t *) tcp_out_buf;
	//------------------ REQUEST -------------
	// fill command fields
	cmd_ptr->dvs_cmd 	= DVS_PROCINFO;
	cmd_ptr->dvs_snode 	= DVS_LOCALNODE;
	cmd_ptr->dvs_dnode 	= nodeid;
	cmd_ptr->dvs_arg1 	= dcid;
	cmd_ptr->dvs_arg2 	= pnr;
	cmd_ptr->dvs_lines 	= 0;
	cmd_ptr->dvs_rcode 	= 0;
	cmd_ptr->dvs_bmnodes= 0;
	cmd_ptr->dvs_paylen	= 0;
	USRDEBUG("PROXYINFO REQUEST dvscmd_ptr:" DVSRQST_FORMAT, DVSRQST_FIELDS(cmd_ptr));
	
	if ( (bytes = send(dvsd_sd, tcp_out_buf, 
				sizeof(dvs_cmd_t)+cmd_ptr->dvs_paylen, 0)) < 0) {
		ERROR_RETURN(errno);
	}
	USRDEBUG("PROXYINFO REQUEST: bytes=%d\n", bytes);

	// ------------- REPLY -----------------
	if( (bytes = recv(dvsd_sd, tcp_in_buf, MAX_MESSLEN, 0 )) < 0) {
		ERROR_RETURN(errno);
	}
	USRDEBUG("PROXYINFO REPLY: bytes=%d\n", bytes);
	
	cmd_ptr = (dvs_cmd_t *) tcp_in_buf;
   	USRDEBUG("PROXYINFO cmd_ptr:" DVSRPLY_FORMAT,DVSRPLY_FIELDS(cmd_ptr));
	if( cmd_ptr->dvs_cmd != DVS_PROCINFO_ACK)
		ERROR_RETURN(EDVSBADVALUE);
	if( cmd_ptr->dvs_rcode != OK) 
		ERROR_RETURN(cmd_ptr->dvs_rcode);
	
	ptr = (char *) cmd_ptr;
	ptr += sizeof(dvs_cmd_t);
	proc_ptr = (proc_usr_t *) ptr;
		
	if (dcid != proc_ptr->p_dcid){
		fprintf(stderr,"Invalid p_dcid (%d)\n", proc_ptr->p_dcid);
		ERROR_EXIT(EDVSBADDCID);
	}	
	
	if (pnr != proc_ptr->p_nr ){
		fprintf(stderr,"Invalid pnr (%d)\n", proc_ptr->p_nr);
		ERROR_EXIT(EDVSBADPROC);
	}	
	
	printf( PROC_USR_FORMAT, PROC_USR_FIELDS(proc_ptr));
	printf( PROC_WAIT_FORMAT, PROC_WAIT_FIELDS(proc_ptr));
	printf( PROC_COUNT_FORMAT, PROC_COUNT_FIELDS(proc_ptr));

	return(cmd_ptr->dvs_rcode);
}

/*===========================================================================*
 *				tcp_connect				     *
 *===========================================================================*/
int tcp_connect(void){
	
    int rcode, i;
    char rmt_ipaddr[INET_ADDRSTRLEN+1];
	char *rmt_ptr;
    struct hostent *dvsd_host;


	// Creating socket file descriptor 
    if ( (dvsd_sd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        ERROR_RETURN(EXIT_FAILURE); 
    } 
	
	USRDEBUG("DVSCMD: connect to DVSD server running at TCP port=%d\n", DVSD_PORT);    

    memset(&dvsd_addr, 0, sizeof(dvsd_addr)); 
    dvsd_addr.sin_family = AF_INET;  
    dvsd_addr.sin_port = htons(DVSD_PORT);  
//	inet_aton("127.0.0.1", &(dvsd_addr.sin_addr.s_addr));

#define  BYNAME	
#ifdef BYNAME	
    dvsd_host = gethostbyname("node0");
	if( dvsd_host == NULL) ERROR_EXIT(-errno);
    dvsd_addr.sin_addr = *((struct in_addr *)dvsd_host->h_addr);
#else  // BYNAME	
	dvsd_addr.sin_addr = inet_addr("127.0.0.1");; 
#endif // BYNAME	
    bzero(&(dvsd_addr.sin_zero), 8);     /* zero the rest of the struct */

	rcode = connect(dvsd_sd, (struct sockaddr *) &dvsd_addr, sizeof(struct sockaddr));
    if (rcode != 0) ERROR_RETURN(errno);
	
	posix_memalign( (void**) &tcp_in_buf, getpagesize(), MAX_MESSLEN );
	if( tcp_in_buf == NULL) ERROR_RETURN (EDVSNOMEM);

	posix_memalign( (void**) &tcp_out_buf, getpagesize(), MAX_MESSLEN );
	if( tcp_out_buf == NULL) ERROR_RETURN (EDVSNOMEM);
	
	return(OK);
}

/*===========================================================================*
 *				dvscmd_init				     *
 *===========================================================================*/
int dvscmd_init(void)
{
	int  dvscmd_pid, i, rcode;
	
	dvscmd_pid = getpid();
   	USRDEBUG( "dvscmd_pid=%d\n",  dvscmd_pid);
	
  	USRDEBUG( "Initialize the call vector to a safe default handler.\n");
  	for (i=0; i<DVS_MAXCMDS; i++) {
		USRDEBUG("Initilizing vector %d on address=%p\n",i, cmd_unused);
      	call_vec[i] = cmd_unused;
  	}

    map(DVS_DVSINIT, 	cmd_dvsinit); 
	USRDEBUG("Initilizing vector %d on address=%p\n",DVS_DVSINIT, cmd_dvsinit);
	map(DVS_DVSEND, 	cmd_dvsend);		
	USRDEBUG("Initilizing vector %d on address=%p\n",DVS_DVSEND, cmd_dvsend);

    map(DVS_DVSINFO, 	cmd_dvsinfo);		
	USRDEBUG("Initilizing vector %d on address=%p\n",DVS_DVSINFO, cmd_dvsinfo);
    map(DVS_DCINFO, 	cmd_dcinfo); 
	USRDEBUG("Initilizing vector %d on address=%p\n",DVS_DCINFO, cmd_dcinfo);
    map(DVS_NODEINFO, 	cmd_nodeinfo);		
	USRDEBUG("Initilizing vector %d on address=%p\n",DVS_NODEINFO, cmd_nodeinfo);
	map(DVS_PROXYINFO, 	cmd_proxyinfo);		
	USRDEBUG("Initilizing vector %d on address=%p\n",DVS_PROXYINFO, cmd_proxyinfo);
	map(DVS_PROCINFO, 	cmd_procinfo);		
	USRDEBUG("Initilizing vector %d on address=%p\n",DVS_PROCINFO, cmd_procinfo);

	
	return(OK);
}

/****************************************************************************************/
/*			DVSCMD 						*/
/****************************************************************************************/
int  main ( int argc, char *argv[] )
{
	int rcode, cmd_nr, result;
	int cmd_nodeid, bytes, cmd_argc, i;
	char *cmd_ptr;
	
	if ( argc < 3 ) {
		print_usage();
 	    exit(1);
    }

	cmd_ptr = argv[2];
	cmd_nodeid = atoi(argv[1]);
	cmd_argc = argc - 2;

	USRDEBUG("cmd_nodeid=%d cmd_ptr=>%s<\n", cmd_nodeid, cmd_ptr);

	// get the command number
	for ( cmd_nr = 1; cmd_nr < DVS_MAXCMDS; cmd_nr++){
		USRDEBUG("cmd_str[%d]=>%s< cmd_ptr=>%s<\n", cmd_nr, cmd_str[cmd_nr], cmd_ptr);
		if ( strcasecmp (cmd_str[cmd_nr], cmd_ptr) )
			continue;
		break;
	}
	
	// invalid command number
	if ( cmd_nr >= DVS_MAXCMDS){
		print_usage();
 	    exit(1);
    }
	
	USRDEBUG("cmd_ptr=%s cmd_argc=%d cmd_nr=%d\n", cmd_ptr, cmd_argc, cmd_nr);
	for( i = 0; i < cmd_argc; i++){
		USRDEBUG("\t cmd_arg[%d]=%s\n", i, argv[i+2]);
	} 
	
	dvscmd_init();

	rcode = tcp_connect();
	if( rcode) {	
		USRDEBUG("tcp_connect rcode=%d\n",rcode);
		ERROR_EXIT(rcode);
	}

   	result = (*call_vec[cmd_nr])(argc, argv);		/* handle the call*/
	
	if(result) ERROR_RETURN(result);
	return(OK);
 }





