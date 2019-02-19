/****************************************************************/
/* 								*/
/* COMMAND LINE INTERFACE */
/****************************************************************/

#include "dvsd.h"

char *tcp_in_buf;
char *tcp_out_buf;

int main(int argc, char *argv[])
 {
        int sockfd, bytes;  
        struct hostent *he;
        struct sockaddr_in their_addr; /* connector's address information */
		dvs_cmd_t *cmd_ptr;
		dvs_usr_t  *dvs_ptr;
		char *ptr;
		
        if (argc != 2) {
            fprintf(stderr,"usage: client hostname\n");
            exit(1);
        }

        if ((he=gethostbyname(argv[1])) == NULL) {  /* get the host info */
            herror("gethostbyname");
            exit(1);
        }

        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            perror("socket");
            exit(1);
        }

		posix_memalign( (void**) &tcp_in_buf, getpagesize(), MAX_MESSLEN );
		if( tcp_in_buf == NULL) ERROR_RETURN (EDVSNOMEM);

		posix_memalign( (void**) &tcp_out_buf, getpagesize(), MAX_MESSLEN );
		if( tcp_out_buf == NULL) ERROR_RETURN (EDVSNOMEM);

        their_addr.sin_family = AF_INET;      /* host byte order */
        their_addr.sin_port = htons(DVSD_PORT);    /* short, network byte order */
        their_addr.sin_addr = *((struct in_addr *)he->h_addr);
        bzero(&(their_addr.sin_zero), 8);     /* zero the rest of the struct */

        if (connect(sockfd, (struct sockaddr *)&their_addr, \
                                              sizeof(struct sockaddr)) == -1) {
            perror("connect");
            exit(1);
        }
		
		cmd_ptr = (dvs_cmd_t *) tcp_out_buf;
	
		//--------------- REQUEST -------------
		// fill command fields
		cmd_ptr->dvs_cmd 	= DVS_DVSINFO;
		cmd_ptr->dvs_snode 	= DVS_LOCALNODE;
		cmd_ptr->dvs_dnode 	= 0;
		cmd_ptr->dvs_lines 	= 0;
		cmd_ptr->dvs_rcode 	= 0;
		cmd_ptr->dvs_bmnodes= 0;
		cmd_ptr->dvs_paylen	= 0;
		USRDEBUG("DVSINFO REQUEST dvscmd_ptr:" DVSCMD_FORMAT, DVSCMD_FIELDS(cmd_ptr));
	
		if (send(sockfd, tcp_out_buf, sizeof(dvs_cmd_t)+ cmd_ptr->dvs_paylen , 0) == -1){
            perror("send");
		    exit (1);
		}
		printf("After the send function \n");

        if ((bytes=recv(sockfd, tcp_in_buf, MAX_MESSLEN, 0)) == -1) {
            perror("recv");
            exit(1);
		}	
		printf("After the recv function bytes=%d \n", bytes);

		cmd_ptr = (dvs_cmd_t *) tcp_in_buf;
		ptr = (char *) cmd_ptr;
		ptr += sizeof(dvs_cmd_t);
		dvs_ptr = (dvs_usr_t  *) ptr;

		USRDEBUG("DVSINFO REPLY dvscmd_ptr:" DVSCMD_FORMAT, DVSCMD_FIELDS(cmd_ptr));
		
		printf(DVS_USR_FORMAT, DVS_USR_FIELDS(dvs_ptr));
		printf(DVS_MAX_FORMAT, DVS_MAX_FIELDS(dvs_ptr));
		printf(DVS_VER_FORMAT, DVS_VER_FIELDS(dvs_ptr));
		
	    close(sockfd);

        return 0;
}

