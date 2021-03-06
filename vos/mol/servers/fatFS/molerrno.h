
#   define _SIGN         -

/* Here are the numerical values of the error numbers. */
#define _NERROR               70  /* number of errors */  

#define EMOLGENERIC      (_SIGN 99)  /* generic error */
#define EMOLPERM         (_SIGN  1)  /* operation not permitted */
#define EMOLNOENT        (_SIGN  2)  /* no such file or directory */
#define EMOLSRCH         (_SIGN  3)  /* no such process */
#define EMOLINTR         (_SIGN  4)  /* interrupted function call */
#define EMOLIO           (_SIGN  5)  /* input/output error */
#define EMOLNXIO         (_SIGN  6)  /* no such device or address */
#define EMOL2BIG         (_SIGN  7)  /* arg list too long */
#define EMOLNOEXEC       (_SIGN  8)  /* exec format error */
#define EMOLBADF         (_SIGN  9)  /* bad file descriptor */
#define EMOLCHILD        (_SIGN 10)  /* no child process */
#define EMOLAGAIN        (_SIGN 11)  /* resource temporarily unavailable */
#define EMOLNOMEM        (_SIGN 12)  /* not enough space */
#define EMOLACCES        (_SIGN 13)  /* permission denied */
#define EMOLFAULT        (_SIGN 14)  /* bad address */
#define EMOLNOTBLK       (_SIGN 15)  /* Extension: not a block special file */
#define EMOLBUSY         (_SIGN 16)  /* resource busy */
#define EMOLEXIST        (_SIGN 17)  /* file exists */
#define EMOLXDEV         (_SIGN 18)  /* improper link */
#define EMOLNODEV        (_SIGN 19)  /* no such device */
#define EMOLNOTDIR       (_SIGN 20)  /* not a directory */
#define EMOLISDIR        (_SIGN 21)  /* is a directory */
#define EMOLINVAL        (_SIGN 22)  /* invalid argument */
#define EMOLNFILE        (_SIGN 23)  /* too many open files in system */
#define EMOLMFILE        (_SIGN 24)  /* too many open files */
#define EMOLNOTTY        (_SIGN 25)  /* inappropriate I/O control operation */
#define EMOLTXTBSY       (_SIGN 26)  /* no longer used */
#define EMOLFBIG         (_SIGN 27)  /* file too large */
#define EMOLNOSPC        (_SIGN 28)  /* no space left on device */
#define EMOLSPIPE        (_SIGN 29)  /* invalid seek */
#define EMOLROFS         (_SIGN 30)  /* read-only file system */
#define EMOLMLINK        (_SIGN 31)  /* too many links */
#define EMOLPIPE         (_SIGN 32)  /* broken pipe */
#define EMOLDOM          (_SIGN 33)  /* domain error    	(from ANSI C std) */
#define EMOLRANGE        (_SIGN 34)  /* result too large	(from ANSI C std) */
#define EMOLDEADLK       (_SIGN 35)  /* resource deadlock avoided */
#define EMOLNAMETOOLONG  (_SIGN 36)  /* file name too long */
#define EMOLNOLCK        (_SIGN 37)  /* no locks available */
#define EMOLNOSYS        (_SIGN 38)  /* function not implemented */
#define EMOLNOTEMPTY     (_SIGN 39)  /* directory not empty */
#define EMOLLOOP         (_SIGN 40)  /* too many levels of symlinks detected */

/* The following errors relate to networking. */
#define EMOLPACKSIZE     (_SIGN 50)  /* invalid packet size for some protocol */
#define EMOLOUTOFBUFS    (_SIGN 51)  /* not enough buffers left */
#define EMOLBADIOCTL     (_SIGN 52)  /* illegal ioctl for device */
#define EMOLBADMODE      (_SIGN 53)  /* badmode in ioctl */
#define EMOLWOULDBLOCK   (_SIGN 54)
#define EMOLBADDEST      (_SIGN 55)  /* not a valid destination address */
#define EMOLDSTNOTRCH    (_SIGN 56)  /* destination not reachable */
#define EMOLISCONN	      (_SIGN 57)  /* all ready connected */
#define EMOLADDRINUSE    (_SIGN 58)  /* address in use */
#define EMOLCONNREFUSED  (_SIGN 59)  /* connection refused */
#define EMOLCONNRESET    (_SIGN 60)  /* connection reset */
#define EMOLTIMEDOUT     (_SIGN 61)  /* connection timed out */
#define EMOLURG	      (_SIGN 62)  /* urgent data present */
#define EMOLNOURG	      (_SIGN 63)  /* no urgent data present */
#define EMOLNOTCONN      (_SIGN 64)  /* no connection (yet or anymore) */
#define EMOLSHUTDOWN     (_SIGN 65)  /* a write call to a shutdown connection */
#define EMOLNOCONN       (_SIGN 66)  /* no such connection */
#define EMOLAFNOSUPPORT  (_SIGN 67)  /* address family not supported */
#define EMOLPROTONOSUPPORT (_SIGN 68) /* protocol not supported by AF */
#define EMOLPROTOTYPE    (_SIGN 69)  /* Protocol wrong type for socket */
#define EMOLINPROGRESS   (_SIGN 70)  /* Operation now in progress */
#define EMOLADDRNOTAVAIL (_SIGN 71)  /* Can't assign requested address */
#define EMOLALREADY      (_SIGN 72)  /* Connection already in progress */
#define EMOLMSGSIZE      (_SIGN 73)  /* Message too long */
#define EMOLNOTSOCK      (_SIGN 74)  /* Socket operation on non-socket */
#define EMOLNOPROTOOPT   (_SIGN 75)  /* Protocol not available */

/* The following are not POSIX errors, but they can still happen. 
 * All of these are generated by the kernel and relate to message passing.
 */
#define EMOLLOCKED      (_SIGN 101)  /* can't send message due to deadlock */
#define EMOLBADCALL     (_SIGN 102)  /* illegal system call number */
#define EMOLBADSRCDST   (_SIGN 103)  /* bad source or destination process */
#define EMOLCALLDENIED  (_SIGN 104)  /* no permission for system call */
#define EMOLDEADSRCDST  (_SIGN 105)  /* source or destination is not alive */
#define EMOLNOTREADY    (_SIGN 106)  /* source or destination is not ready */
#define EMOLBADREQUEST  (_SIGN 107)  /* destination cannot handle request */
#define EMOLSRCDIED     (_SIGN 108)  /* source just died */
#define EMOLDSTDIED     (_SIGN 109)  /* destination just died */
#define EMOLTRAPDENIED  (_SIGN 110)  /* IPC trap not allowed */
#define EMOLDONTREPLY   (_SIGN 201)  /* pseudo-code: don't send a reply */

/* the following ar MOL error codes */
#define EMOLBADNODEID 	(_SIGN 301)  /* Bad NODE ID */
#define EMOLBADVMID 	(_SIGN 302)  /* Bad VM ID */
#define EMOLVMNOTRUN 	(_SIGN 303)  /* The VM is not running */
#define EMOLBADPROC  	(_SIGN 304)  /* Bad Process Number */
#define EMOLBADPID  	(_SIGN 305)  /* Bad Process ID */
#define EMOLENDPOINT  	(_SIGN 306)  /* Bad Process Endpoint */
#define EMOLNOPROXY 	(_SIGN 307)  /* The IPC proxy is not running */
#define EMOLRMTPROC 	(_SIGN 308)  /* The process is REMOTE */
#define EMOLLCLPROC 	(_SIGN 309)  /* The process is LOCAL */
#define EMOLNOTBIND 	(_SIGN 310)  /* The process has not BINDed */
#define EMOLPROXYRUN 	(_SIGN 311)  /* The IPC proxy is already running */
#define EMOLACKDST 	(_SIGN 312)  /* The IPC proxy sent local process a BAD destination ACK  */
#define EMOLACKSRC 	(_SIGN 313)  /* The IPC proxy sent local process a BAD source ACK  */
#define EMOLACKWAIT 	(_SIGN 314)  /* The IPC proxy sent local process that is not waiting for it  */
#define EMOLNODEBUSY 	(_SIGN 315)  /* NODE is BUSY */
#define EMOLNONODE 	(_SIGN 316)  /* NODE does not exists or is not in list */
#define EMOLDRVSBUSY 	(_SIGN 317)  /* The DRVS is BUSY - Can't assign new local node ID */
#define EMOLDRVSINIT 	(_SIGN 318)  /* The DRVS has not been initialized */
#define EMOLNOVMNODE 	(_SIGN 319)  /* The NODE is not included into the VM list of nodes */
#define EMOLPROCRUN 	(_SIGN 320)  /* The copy source/destination process is running */
#define EMOLNOMSG 	(_SIGN 321)  /*  No message  - list empty	*/
#define EMOLVMRUN 	(_SIGN 323)  /* The VM is already running */
#define EMOLPROCSTS (_SIGN 324)  	/* Bad process status  */
#define EMOLNODEFREE (_SIGN 325)  /* NODE is FREE */
#define EMOLVMNODE 	(_SIGN 326)  /* The NODE is ALLREADY included into the VM list of nodes */
#define EMOLBADPROXY (_SIGN 327)  /* The NODE is ALLREADY included into the VM list of nodes */
#define EMOLONCOPY	 (_SIGN 328)  	/* The process is in ONCOPY state  */
#define EMOLENQUEUED	 (_SIGN 329)  	/* The process descriptor is enqueued and cant be used */
#define EMOLOVERRUN	 (_SIGN 330)  	/* An operation couse an overrun of some system resource */













