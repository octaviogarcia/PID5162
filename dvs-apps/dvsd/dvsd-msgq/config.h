#ifndef CONFIG_H
#define CONFIG_H

#define USE_DVSINIT	   	   1
#define USE_DVSINFO	   	   1
#define USE_DVSEND	   	   1

#define OK 	0
#define TRUE 1
/*
* Member States for the local Finite State Machine (FSM) 
*/

#define	   	STS_NEW				0x0000
#define		STS_RUNNING			0x0001
#define		STS_DISCONNECTED	0x0002
#define		STS_LEAVE 			STS_DISCONNECTED

#define MAX_VSSETS      10
#define MAX_MEMBERS     (sizeof(unsigned long) * 8)
#define SYS_DELAY		2
#define NULL_NODE  		LOCALNODE

#define SLOT_TIMEOUT_SEC	5
#define SLOT_TIMEOUT_MSEC	0

#define 	QUEUEBASE	7000

#endif /* CONFIG_H */

