#define _MULTI_THREADED
#define _GNU_SOURCE     

#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <setjmp.h>
#include <pthread.h>
#include <sched.h>
#include <netdb.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <sys/sysinfo.h> 
#include <sys/stat.h>
#include <sys/syscall.h> 
#include <sys/mman.h>
#include <fcntl.h>
#include <malloc.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#define DVS_USERSPACE	1

#include "../../../../include/com/dvs_config.h"
#include "../../../../include/com/config.h"
#include "../../../../include/com/const.h"
#include "../../../../include/com/types.h"
#include "../../../../include/com/timers.h"
#include "../../../../include/com/dvs_usr.h"
#include "../../../../include/com/dc_usr.h"
#include "../../../../include/com/node_usr.h"
#include "../../../../include/com/proc_usr.h"
#include "../../../../include/com/proc_sts.h"
#include "../../../../include/com/com.h"
#include "../../../../include/com/ipc.h"
#include "../../../../include/com/cmd.h"
#include "../../../../include/com/proxy_usr.h"
#include "../../../../include/com/proxy_sts.h"
#include "../../../../include/com/dvs_errno.h"
#include "../../../../include/com/endpoint.h"
#include "../../../../include/com/priv_usr.h"
#include "../../../../include/com/priv.h"
#include "../../../../include/com/stub_dvkcall.h"

#include "../debug.h"
#include "../macros.h"

#define _POSIX_SOURCE      1	/* tell headers to include POSIX stuff */

#define 	RAD_NONE	0 
#define 	RAD_CONFIG	(1 << 0)
#define 	RAD_BIND	(1 << 1)
#define 	RAD_UNBIND	(1 << 2)
#define 	RAD_DCID	(1 << 3)
#define 	RAD_GROUP	(1 << 4)









