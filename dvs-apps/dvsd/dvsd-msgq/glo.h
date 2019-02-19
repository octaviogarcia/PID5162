#ifdef _DVSD
#define EXTERN
#else
#define EXTERN extern
#endif

EXTERN int 		local_nodeid;
EXTERN int 		pagesize;
EXTERN int 		dvs_pid;
EXTERN dvs_cmd_t dvscmd, *dvscmd_ptr;
EXTERN sigjmp_buf senv;
EXTERN unsigned long init_nodes;		
EXTERN mailbox 	dvsmbox;		/* SPREAD MAILBOX for SYSTASK */	
EXTERN char		gcsmsg_in[MAX_MESSLEN];
EXTERN char		gcsmsg_out[MAX_MESSLEN];
EXTERN  char    Spread_name[80];
EXTERN  char    Private_group[MAX_GROUP_NAME];
EXTERN  char    first_name[MAX_GROUP_NAME];
EXTERN  char	User[80];
EXTERN  pthread_t gcs_thread; 		
EXTERN	int		FSM_state;		/* Finite State Machine state */
EXTERN	int		SP_bytes;		/* bytes returned by SP_receive */

EXTERN  membership_info 	memb_info;
EXTERN  vs_set_info      	vssets[MAX_VSSETS];
EXTERN  unsigned int    	my_vsset_index;
EXTERN  int              	num_vs_sets;
EXTERN  char            	members[MAX_MEMBERS][MAX_GROUP_NAME];
EXTERN	char		   		sp_members[MAX_MEMBERS][MAX_GROUP_NAME];
EXTERN	int			   		sp_nr_mbrs;
EXTERN	unsigned long int	bm_init;		/* bitmap of initialized  members */
EXTERN 	time_t 				last_rqst;		/* time of last slot request */

EXTERN struct msqid_ds mq_in_ds;
EXTERN struct msqid_ds mq_out_ds;
EXTERN int mq_in;
EXTERN int mq_out;
EXTERN  struct msgbuf_s *mq_in_buf;
EXTERN  struct msgbuf_s *mq_out_buf;

#ifdef _DVSD
char group_name[] = "DVS";
pthread_mutex_t dvs_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t dvs_barrier = PTHREAD_COND_INITIALIZER;
#else
extern char group_name[];
extern pthread_mutex_t dvs_mutex;
extern pthread_cond_t dvs_barrier;
#endif


		

