/****************************************************************/
/* 				DVSCMD				*/
/* COMMAND LINE INTERFACE */
/****************************************************************/

#define _DVSCMD
#include "dvsd.h"

struct msqid_ds mq_in_ds;
struct msqid_ds mq_out_ds;
int mq_in;
int mq_out;
struct msgbuf_s *mq_in_buf;
struct msgbuf_s *mq_out_buf;
int local_nodeid;
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
	dvs_cmd_t *cmd_ptr;
	dvs_usr_t  *dvs_ptr;
	char *ptr;
	
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

	// set the pointer to payload: dvs_usr_t 
	cmd_ptr = (dvs_cmd_t *) &mq_out_buf->mtext;
	ptr = (char *) cmd_ptr;
	ptr += sizeof(dvs_cmd_t);
	dvs_ptr = (dvs_usr_t*) ptr;
	
	// fill command fields
	cmd_ptr->dvs_cmd 	= DVS_DVSINIT;
	cmd_ptr->dvs_snode 	= DVS_LOCALNODE;
	cmd_ptr->dvs_dnode 	= nodeid;
	cmd_ptr->dvs_lines 	= 1;
	cmd_ptr->dvs_rcode 	= 0;
	cmd_ptr->dvs_bmnodes 	= 0;
	cmd_ptr->dvs_paylen	= sizeof(dvs_usr_t);
	
	// copy from local DVS user struct to msgq message payload
	memcpy( dvs_ptr, &dvs_lcl, sizeof(dvs_usr_t));
	
	DVSDDEBUG("DVSINIT REQUEST dvscmd_ptr:" DVSCMD_FORMAT, DVSCMD_FIELDS(cmd_ptr));
	DVSDDEBUG(DVS_USR_FORMAT, DVS_USR_FIELDS(dvs_ptr));
	DVSDDEBUG(DVS_MAX_FORMAT, DVS_MAX_FIELDS(dvs_ptr));
	DVSDDEBUG(DVS_VER_FORMAT, DVS_VER_FIELDS(dvs_ptr));
	
	// send message to local DVSD 
	mq_out_buf->mtype = 0x0001;
	bytes = msgsnd(mq_out, mq_out_buf, sizeof(dvs_cmd_t)+sizeof(dvs_usr_t), 0); 
	DVSDDEBUG("msgsnd bytes=%d\n", bytes);
	if(bytes < 0) {
		DVSERR(errno);
		exit(1);
	}
	
	// wait for reply
	DVSDDEBUG("DVSINIT waiting for reply\n");
	mq_in_buf->mtype = 0x0001;
	bytes = msgrcv(mq_in, mq_in_buf, MAX_MESSLEN, 0 , 0 );
	DVSDDEBUG("msgrcv bytes=%d\n", bytes);
	if(bytes < 0) {
		DVSERR(errno);
		exit(1);
	}
	cmd_ptr = (dvs_cmd_t *) &mq_in_buf->mtext;
   	DVSDDEBUG("DVSINIT cmd_ptr:" DVSCMD_FORMAT,DVSCMD_FIELDS(cmd_ptr));
	
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
	
	cmd_ptr = (dvs_cmd_t *) &mq_out_buf->mtext;
	
	// fill command fields
	cmd_ptr->dvs_cmd 	= DVS_DVSINFO;
	cmd_ptr->dvs_snode 	= DVS_LOCALNODE;
	cmd_ptr->dvs_dnode 	= nodeid;
	cmd_ptr->dvs_lines 	= 0;
	cmd_ptr->dvs_rcode 	= 0;
	cmd_ptr->dvs_bmnodes 	= 0;
	cmd_ptr->dvs_paylen	= 0;
	
	DVSDDEBUG("DVSINFO REQUEST dvscmd_ptr:" DVSCMD_FORMAT, DVSCMD_FIELDS(cmd_ptr));
	
	// send message to local DVSD 
	mq_out_buf->mtype = 0x0001;
	bytes = msgsnd(mq_out, mq_out_buf, sizeof(dvs_cmd_t), 0); 
	DVSDDEBUG("msgsnd bytes=%d\n", bytes);
	if(bytes < 0) {
		DVSERR(errno);
		exit(1);
	}
	
	// wait for reply
	DVSDDEBUG("DVSINFO waiting for reply\n");
	mq_in_buf->mtype = 0x0001;
	bytes = msgrcv(mq_in, mq_in_buf, MAX_MESSLEN, 0 , 0 );
	DVSDDEBUG("msgrcv bytes=%d\n", bytes);
	if(bytes < 0) {
		DVSERR(errno);
		exit(1);
	}
	
	cmd_ptr = (dvs_cmd_t *) &mq_in_buf->mtext;
   	DVSDDEBUG("DVSINFO cmd_ptr:" DVSCMD_FORMAT,DVSCMD_FIELDS(cmd_ptr));
	if( cmd_ptr->dvs_rcode != cmd_ptr->dvs_dnode) 
		ERROR_RETURN(cmd_ptr->dvs_rcode);
	
	// check if the size of the data read is correct
	if( bytes != (sizeof(dvs_cmd_t)+sizeof(dvs_usr_t)))
		ERROR_RETURN(EMOLMSGSIZE);
	
	ptr = (char *) cmd_ptr;
	ptr += sizeof(dvs_cmd_t);
	dvs_ptr = (dvs_usr_t*) ptr;

	printf(DVS_USR_FORMAT, DVS_USR_FIELDS(dvs_ptr));
	printf(DVS_MAX_FORMAT, DVS_MAX_FIELDS(dvs_ptr));
	printf(DVS_VER_FORMAT, DVS_VER_FIELDS(dvs_ptr));
	
	return(OK);
}

/*===========================================*
 *				cmd_dvsend				     *
 *===========================================*/
int cmd_dvsend(int argc, char *argv[]) 
{
	int nodeid, bytes;
	dvs_cmd_t *cmd_ptr;
	
	if( argc != 3){
		print_usage();
		ERROR_EXIT(EMOLINVAL);
	}
		
	nodeid = atoi(argv[2]);
	if (nodeid < 0 || nodeid > MAX_MEMBERS){
		fprintf(stderr,"Invalid nodeid (%d)\n", nodeid);
		ERROR_EXIT(EMOLBADNODEID)
	}
	
	cmd_ptr = (dvs_cmd_t *) &mq_out_buf->mtext;
	
	// fill command fields
	cmd_ptr->dvs_cmd 	= DVS_DVSEND;
	cmd_ptr->dvs_snode 	= DVS_LOCALNODE;
	cmd_ptr->dvs_dnode 	= nodeid;
	cmd_ptr->dvs_lines 	= 0;
	cmd_ptr->dvs_rcode 	= 0;
	cmd_ptr->dvs_bmnodes 	= 0;
	cmd_ptr->dvs_paylen	= 0;
	
	DVSDDEBUG("DVSEND REQUEST dvscmd_ptr:" DVSCMD_FORMAT, DVSCMD_FIELDS(cmd_ptr));
	
	// send message to local DVSD 
	mq_out_buf->mtype = 0x0001;
	bytes = msgsnd(mq_out, mq_out_buf, sizeof(dvs_cmd_t), 0); 
	DVSDDEBUG("msgsnd bytes=%d\n", bytes);
	if(bytes < 0) {
		DVSERR(errno);
		exit(1);
	}
	
	// wait for reply
	DVSDDEBUG("DVSEND waiting for reply\n");
	mq_in_buf->mtype = 0x0001;
	bytes = msgrcv(mq_in, mq_in_buf, MAX_MESSLEN, 0 , 0 );
	DVSDDEBUG("msgrcv bytes=%d\n", bytes);
	if(bytes < 0) {
		DVSERR(errno);
		exit(1);
	}
	cmd_ptr = (dvs_cmd_t *) &mq_in_buf->mtext;
   	DVSDDEBUG("DVSEND cmd_ptr:" DVSCMD_FORMAT,DVSCMD_FIELDS(cmd_ptr));

	if( cmd_ptr->dvs_rcode != cmd_ptr->dvs_dnode) 
		ERROR_RETURN(cmd_ptr->dvs_rcode);
	return(cmd_ptr->dvs_rcode);
}

/*===========================================================================*
 *				msgq_init				     *
 *===========================================================================*/
int msgq_init(void){
	
	mq_in = msgget(QUEUEBASE+1, IPC_CREAT | 0x660);
	if ( mq_in < 0) {
		if ( errno != EEXIST) {
			DVSERR(mq_in);
			return(mq_in);
		}
		DVSDDEBUG( "The queue with key=%d already exists\n",QUEUEBASE+1);
		mq_in = msgget( (QUEUEBASE+1), 0);
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

	mq_out = msgget(QUEUEBASE, IPC_CREAT | 0x660);
	if ( mq_out < 0) {
		if ( errno != EEXIST) {
			DVSERR(mq_out);
			return(mq_out);
		}
		DVSDDEBUG("The queue with key=%d already exists\n",QUEUEBASE);
		mq_out = msgget( (QUEUEBASE), 0);
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
	
	rcode = msgq_init();
	if( rcode) {	
		DVSDDEBUG("msgq_init rcode=%d\n",rcode);
		ERROR_EXIT(rcode);
	}

	incmd_ptr  = (dvs_cmd_t *) &mq_in_buf->mtext;
	outcmd_ptr = (dvs_cmd_t *) &mq_out_buf->mtext;

	dvscmd_init();
	
   	result = (*call_vec[cmd_nr])(argc, argv);		/* handle the call*/
	
	if(result) ERROR_RETURN(result);
	return(OK);
 }





