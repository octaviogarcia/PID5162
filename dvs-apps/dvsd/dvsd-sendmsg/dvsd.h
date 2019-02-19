#define _MULTI_THREADED
#define _GNU_SOURCE     
#define  MOL_USERSPACE	1

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
#include <ansi.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <sys/sysinfo.h> 
#include <sys/stat.h>
#include <sys/syscall.h> 
#include <sys/mman.h>
#include <sys/msg.h>

#include <fcntl.h>
#include <malloc.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include "./include/com/dvs_config.h"
#include "./include/com/config.h"
// #include "./include/com/ansi.h"
#include "./include/com/dvs_errno.h"
#include "./include/com/dvs_usr.h"
#include "./include/com/kipc.h"
#include "./include/com/stub_dvkcall.h"

#include "sp.h"

#include "debug.h"
#include "macros.h"

#include "config.h"
#include "dvscmd.h"
//#include "table.h"
#include "proto.h"
#include "msgq.h"






