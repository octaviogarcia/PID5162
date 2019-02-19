

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kallsyms.h>
#include <linux/kprobes.h>
#include <linux/init.h>
#include <linux/kernel.h>	/* printk() */
#include <linux/slab.h>		/* kmalloc() */
#include <linux/fs.h>		/* everything... */
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */
#include <linux/proc_fs.h>
#include <linux/debugfs.h>
#include <linux/fcntl.h>	/* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/cdev.h>
#include <linux/cpu.h>
#include <linux/sched.h>
#include <linux/uaccess.h>	/* copy_*_user */
#include <linux/cpumask.h>
#include <linux/syscalls.h>
#include <linux/capability.h>

#include <uapi/linux/uio.h>

#include "../include/com/dvs_config.h"
#include "../include/com/config.h"
#include "../include/com/com.h"
#include "../include/com/const.h"
#include "../include/com/endpoint.h"
#include "../include/com/cmd.h"
#include "../include/com/priv_usr.h"
#include "../include/com/priv.h"
#include "../include/com/dc_usr.h"
#include "../include/com/node_usr.h"
#include "../include/com/proc_sts.h"
#include "../include/com/proc_usr.h"
#include "../include/com/proc.h"
#include "../include/com/proxy_sts.h"
#include "../include/com/proxy_usr.h"
#include "../include/com/proxy.h"
#include "../include/com/macros.h"
#include "../include/com/dvs_usr.h"
#include "../include/com/dvk_calls.h"
#include "../include/com/dvk_ioctl.h"
#include "../include/com/dvs_errno.h"
#include "../include/dvk/dvk_ioparm.h"

#include "dvk_debug.h"
#include "dvk_macros.h"
#include "dvk_proto.h"
#include "dvk_glo.h"
#include "dvk_ioproto.h"
#include "dvk_newproto.h"
#include "reljmp.h"

extern atomic_t local_nodeid;



