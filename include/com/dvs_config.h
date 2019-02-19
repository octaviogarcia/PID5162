
#ifndef _COM_DVSCONFIG_H
#define _COM_DVSCONFIG_H 1

/*===========================================================================*
 *		This section contains user-settable parameters		     *
 *===========================================================================*/
 
#define _NR_FIXED_TASKS 3					/* CLOCK, IDLE, SLOTS*/
#define _NR_NODES 		32
#define _NR_SYSTASKS	(_NR_NODES)			/* Threads por SYSTASK  */
#define _NR_TASKS		(_NR_FIXED_TASKS+_NR_SYSTASKS)	/* HARDWARE, CLOCK, IDLE + SYSTASKS (32+3) */
#define _NR_PROCS		(256 - _NR_TASKS)   /* 256 - 35 = 221 */
#define _NR_SERVERS		(32-_NR_FIXED_TASKS)		/* (32-3) = 29*/
#define _NR_SYS_PROCS	(_NR_SERVERS+_NR_TASKS)   	/* 32+3+32-3 = 32+32 = 64 */
#define _NR_USR_PROCS   (_NR_PROCS - _NR_SYS_PROCS)	/* (221-29) = 192 */
#define _NR_DCS	     32

#if (_NR_TASKS >= 300)  
#error "_NR_TASKS must be lower than -EDVSERRCODE (300)"
#endif

#endif /* _COM_DVSCONFIG_H */
