
#define 	DVS_ACKNOWLEDGE 0x1000
#define 	DVS_ALLNODES	0xFFFF
#define 	DVS_LOCALNODE	(-1)

/* COMMANDS 	*/
  enum dvs_cmd_enum{
        DVS_NONE      =  0,	/* NO COMMAND  							*/
		DVS_DVSINIT	  =  1,	
		DVS_DVSINFO	  =  2,	
		DVS_DVSEND	  =  3,	
		DVS_MAXCMDS   =  4,	/* THIS MUST BE THE LAST COMMAND */
  };

#define DVS_DVSINIT_ACK		(DVS_DVSINIT	| DVS_ACKNOWLEDGE) 
#define DVS_DVSINFO_ACK		(DVS_DVSINFO 	| DVS_ACKNOWLEDGE) 
#define DVS_DVSEND_ACK		(DVS_DVSEND 	| DVS_ACKNOWLEDGE) 
  

struct dvs_cmd_s {
	int	dvs_cmd;		
	int	dvs_snode;	/* source node				*/
	int	dvs_dnode;	/* destination node			*/
	int dvs_rcode;	/* return code 				*/
	int dvs_lines;	/* # of lines in the payload*/
  	int dvs_paylen;	/* payload len 				*/
	unsigned long dvs_bmnodes;
};
typedef struct dvs_cmd_s dvs_cmd_t;

#define DVSCMD_FORMAT "dvs_cmd=0x%X dvs_snode=%d dvs_dnode=%d dvs_rcode=%d dvs_lines=%d dvs_paylen=%d dvs_bmnodes=0x%lX  \n" 
#define DVSCMD_FIELDS(p) p->dvs_cmd, p->dvs_snode, p->dvs_dnode, p->dvs_rcode, p->dvs_lines, p->dvs_paylen, p->dvs_bmnodes

#ifdef _DVSCMD
char *cmd_str[] = {
	"NONE",
	"DVSINIT",
	"DVSINFO",
	"DVSEND",
	"MAXCMDS"
};
#endif







