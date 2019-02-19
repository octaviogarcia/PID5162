#include "radcmd.h"

extern char *optarg;
extern int optind, opterr, optopt;

dc_usr_t  dcu, *dc_ptr;
dvs_usr_t dvs, *dvs_ptr;
int local_nodeid;

int dcid, nodeid;
char *const deamon_args;
int radcmd_sd;
struct sockaddr_in rad_addr;
struct hostent *rad_host;

udp_buf_t udpbuf_in, udpbuf_out;
char  *cmd_line;

#define FORK_WAIT_MS 1000

print_usage(char *argv0){
	fprintf(stderr,"Usage:\n");
	fprintf(stderr,"\t%s -c <config_file> \n", argv0 );
	fprintf(stderr,"\t\t Send configuration file commands to radard\n");	
	fprintf(stderr,"\t%s -b <clt_ep> -d <dcid> -g <group_name> <client_prog> <args> \n", argv0 );
	fprintf(stderr,"\t\t Tell remote nodes of group <group_name> \n");
	fprintf(stderr,"\t\t to bind the client program on DC <dcid> with endpoint <clt_ep>\n");	
	fprintf(stderr,"\t%s -u <clt_ep> -d <dcid> -g <group_name> \n", argv0 );
	fprintf(stderr,"\t\t Tell remote nodes of group <group_name> \n");
	fprintf(stderr,"\t\t to unbind endpoint <clt_ep> from DC with <dcid>\n");	;
	ERROR_EXIT(EDVSINVAL);	
}

/*===========================================================================*
 *				main					     					*
 *===========================================================================*/
int main ( int argc, char *argv[] )
{
	int rcode, opt, i, arg_len, bytes, len;
	char *arg_ptr, *iptr, *optr;
	message *rqst_ptr,*reply_ptr;
    int bytes_to_read = _POSIX_ARG_MAX;
	int nbytes, was_space, fd_tty;
	
	if ( argc < 3  ) {
		CMDDEBUG("argc=%d\n", argc);
		for( i = 0; i < argc ; i++) {
			CMDDEBUG("argv[%d]=%s\n", i, argv[i]);
		}
		print_usage("argv[0]");
 	    exit(1);
    }

	cmd_type = RAD_NONE;
    while ((opt = getopt(argc, argv, "cbdgu")) != -1) {
        switch (opt) {
			case 'c':
				if( cmd_type != RAD_NONE || argc != 3 )
					print_usage(argv[0]);
				cmd_type = RAD_CONFIG;
				break;
			case 'b':
				if( argc < 8 )
					print_usage(argv[0]);
				cmd_type |= RAD_BIND;
				break;
			case 'd':
				cmd_type |= RAD_DCID;
				break;
			case 'g':
				cmd_type |= RAD_GROUP;
				break;
			case 'u':
				if( argc != 7 )
					print_usage(argv[0]);
				cmd_type |= RAD_UNBIND;
				break
			default: /* '?' */
				print_usage(argv[0]);
				break;
        }
    }
	
	CMDDEBUG("getopt cmd_type=%d\n", cmd_type);
	switch (cmd_type){
		case RAD_CONFIG:
			break;
		case (RAD_BIND | RAD_DCID | RAD_GROUP):
			break;
		case (RAD_UNBIND | RAD_DCID | RAD_GROUP):
			break;			
		default:
			print_usage(argv[0]);			
	}
	
	rcode = udp_init(argv[2]);
	if(rcode) {	
		CMDDEBUG("udp_init rcode=%d\n",rcode);
		ERROR_EXIT(rcode);
	}
	
	switch (cmd_type){
		case RAD_CONFIG:
			rcode = rad_config(argc, argv);		
			break;
		case (RAD_BIND | RAD_DCID | RAD_GROUP):
			rcode = rad_bind(argc, argv);
			break;
		case (RAD_UNBIND | RAD_DCID | RAD_GROUP):
			rcode = rad_unbind(argc, argv);		
			break;			
		default:
			assert(1);
			break;
	}
	
#ifdef HASTA_AQUI_LLEGUE 
	
	/* Dump the argv[]  into a copy buffer */
	CMDDEBUG("argc=%d \n",argc);

	arg_ptr = udpbuf_in.udp_u.mnx.arg_v;
	bzero(arg_ptr, _POSIX_ARG_MAX-sizeof(int)-sizeof(message));
	arg_len = 0;
	if( argc == 7) { //means open an input line 
//		if() {
//			dup2(fileno(someotherfile), STDOUT_FILENO);
//			dup2(fileno(somethirdopenfile), STDERR_FILENO);
//		}
		puts("Please enter a command:\n");
		nbytes = getline(&arg_ptr, &bytes_to_read, stdin);
		arg_ptr[nbytes] = 0; // Replace LF
		--nbytes;
	} else {
		strcpy(arg_ptr,argv[7]);
		nbytes = strlen(arg_ptr);
	}

	CMDDEBUG("nbytes=%d strlen=%d >%s<\n",nbytes, strlen(arg_ptr), arg_ptr);
#ifdef  TOKENIZE
	arg_len = 0;
	iptr = optr = arg_ptr;
	// convert a string into a sequence of strings (spaces replaced by zeros)
	for (was_space = TRUE; *iptr != 0; iptr++) {
		if (isspace(*iptr)) {
			if(was_space == FALSE){
				*optr = '\0';
				arg_len++;
				optr++;
			}
			was_space = TRUE;
		} else {
			was_space = FALSE;
			optr++;
			arg_len++;
		}
	}
	arg_ptr =  optr;
	arg_len++;

	// set a zero at the end of the sequence of strings 
	arg_ptr++;
	*arg_ptr = '\0';	
	arg_len++;
#else // TOKENIZE
	arg_len = nbytes;
#endif // TOKENIZE

	CMDDEBUG("arg_len=%d\n",arg_len);
	CMDDEBUG("arg_v= >%s<\n", udpbuf_in.udp_u.mnx.arg_v);

	/* build the request message to insert into the RS input message queue */
	rqst_ptr 	= &udpbuf_in.udp_u.mnx.mnx_msg;
	reply_ptr 	= &udpbuf_out.udp_u.mnx.mnx_msg;
	udpbuf_in.mtype 		= RAD_UP;
	rqst_ptr->m_type 		= RAD_UP;
	rqst_ptr->M7_ENDPT1		= deamon_ep;	// m7_i1	
	rqst_ptr->M7_BIND_TYPE 	= cmd_type;	// m7_i2	
	if( cmd_type == RMT_BIND || cmd_type == BKUP_BIND)
		rqst_ptr->M7_NODEID		= nodeid;	// m7_i3		
	else
		rqst_ptr->M7_MNXPID = mnxpid;		// m7_i3
	rqst_ptr->M7_LEN		= (char * ) arg_len; // m7_i4
	rqst_ptr->M7_RUNNODE	= (int) deamon_nodeid; 	
	CMDDEBUG(MSG7_FORMAT, MSG7_FIELDS(rqst_ptr));
	bytes = arg_len + sizeof(message) + sizeof(int) + 1;
	
	/* rad_mq_in is the RS input message queue */
	CMDDEBUG("Sending request to RS bytes=%d\n",bytes);
    bytes = sendto(radcmd_sd, &udpbuf_in, bytes, 0, (struct sockaddr*) &rad_addr, sizeof(rad_addr));
	if( bytes < 0) {
		CMDDEBUG("msgsnd errno=%d\n",errno);
		ERROR_EXIT(errno);
	}
		
	/* rad_mq_in is the RS output message queue */
	CMDDEBUG("Receiving reply from RS\n");
	len = sizeof(struct sockaddr_in);
   	bytes = recvfrom(radcmd_sd, &udpbuf_out, sizeof(udp_buf_t), 
					0, (struct sockaddr*) &rad_addr, &len );
	if( bytes < 0) {
		CMDDEBUG("msgrcv errno=%d\n",errno);
		ERROR_EXIT(errno);
	}
	CMDDEBUG(MSG7_FORMAT, MSG7_FIELDS(reply_ptr));

	CMDDEBUG("m_type=0x%X bytes=%d\n", reply_ptr->m_type, bytes);	

#endif // HASTA_AQUI_LLEGUE 

	exit(0);
}

/*===========================================================================*
 *				udp_init					     *
 *===========================================================================*/
int udp_init( char *host_ptr)
{
	int rad_port, rcode, i;
    char rad_ipaddr[INET_ADDRSTRLEN+1];

	rad_port = RAD_BASE_PORT + (dcid * dvs_ptr->d_nr_nodes) + local_nodeid;
	
	CMDDEBUG("RS may be listening on host %s at port=%d\n",  host_ptr, rad_port);    

    rad_addr.sin_family = AF_INET;  
    rad_addr.sin_port = htons(rad_port);  

    rad_host = gethostbyname(host_ptr);
	if( rad_host == NULL) ERROR_EXIT(errno);
	
	for( i =0; rad_host->h_addr_list[i] != NULL; i++) {
		CMDDEBUG("RS host address %i: %s\n", i, 
			inet_ntoa( *( struct in_addr*)(rad_host->h_addr_list[i])));
	}
	
    if((inet_pton(AF_INET,	
		inet_ntoa( *( struct in_addr*)(rad_host->h_addr_list[0])), 
		(struct sockaddr*) &rad_addr.sin_addr)) <= 0)
    	ERROR_RETURN(errno);

    inet_ntop(AF_INET, (struct sockaddr*) &rad_addr.sin_addr, rad_ipaddr, INET_ADDRSTRLEN);
	CMDDEBUG("RS is running on  %s at IP=%s\n", host_ptr, rad_ipaddr);    

    if ( (radcmd_sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
       	ERROR_EXIT(errno)
    }
	
	rcode = connect(radcmd_sd, (struct sockaddr *) &rad_addr, sizeof(rad_addr));
    if (rcode != 0) ERROR_RETURN(errno);
    return(OK);
}
