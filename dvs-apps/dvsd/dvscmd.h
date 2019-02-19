
#define 	DVS_ACKNOWLEDGE 0x1000
#define 	DVS_ALLNODES	0xFFFF
#define 	DVS_LOCALNODE	(-1)

/* COMMANDS 	*/
  enum dvs_cmd_enum{
        DVS_NONE      =  0,	/* NO COMMAND  							*/

		DVS_DVSINIT	  =  1,	
		DVS_DVSEND	  =  2,	

		DVS_DVSINFO  =  3,	
		DVS_DCINFO	  =  4,	
		DVS_NODEINFO  =  5,	
		DVS_PROXYINFO =  6,	
		DVS_PROCINFO  =  7,	

		
		DVS_MAXCMDS   =  8,	/* THIS MUST BE THE LAST COMMAND */
  };

#define DVS_DVSINIT_ACK		(DVS_DVSINIT	| DVS_ACKNOWLEDGE) 
#define DVS_DVSEND_ACK		(DVS_DVSEND 	| DVS_ACKNOWLEDGE) 

#define DVS_DVSINFO_ACK		(DVS_DVSINFO 	| DVS_ACKNOWLEDGE) 
#define DVS_DCINFO_ACK		(DVS_DCINFO 	| DVS_ACKNOWLEDGE) 
#define DVS_NODEINFO_ACK	(DVS_NODEINFO 	| DVS_ACKNOWLEDGE) 
#define DVS_PROXYINFO_ACK	(DVS_PROXYINFO 	| DVS_ACKNOWLEDGE) 
#define DVS_PROCINFO_ACK	(DVS_PROCINFO 	| DVS_ACKNOWLEDGE) 
  

struct dvs_cmd_s {
	int	dvs_cmd;		
	int	dvs_snode;	/* source node				*/
	int	dvs_dnode;	/* destination node			*/
	int dvs_rcode;	/* return code 				*/
	int dvs_lines;	/* # of lines in the payload*/
  	int dvs_paylen;	/* payload len 				*/
	unsigned long dvs_bmnodes;
	int dvs_arg1;
	int dvs_arg2;
};
typedef struct dvs_cmd_s dvs_cmd_t;

#define DVSRQST_FORMAT "dvs_cmd=0x%X dvs_snode=%d dvs_dnode=%d dvs_arg1=%d dvs_arg2=%d dvs_lines=%d dvs_paylen=%d dvs_bmnodes=0x%lX\n" 
#define DVSRQST_FIELDS(p) p->dvs_cmd, p->dvs_snode, p->dvs_dnode, p->dvs_arg1,p->dvs_arg2, p->dvs_lines, p->dvs_paylen, p->dvs_bmnodes

#define DVSRPLY_FORMAT "dvs_cmd=0x%X dvs_snode=%d dvs_dnode=%d dvs_rcode=%d dvs_lines=%d dvs_paylen=%d dvs_bmnodes=0x%lX  \n" 
#define DVSRPLY_FIELDS(p) p->dvs_cmd, p->dvs_snode, p->dvs_dnode, p->dvs_rcode, p->dvs_lines, p->dvs_paylen, p->dvs_bmnodes

#ifdef _DVSCMD
char *cmd_str[] = {
	"NONE",
	"DVSINIT",
	"DVSEND",
	"DVSINFO",
	"DCINFO",
	"NODEINFO",
	"PROXYINFO",
	"PROCINFO",
};
#endif







