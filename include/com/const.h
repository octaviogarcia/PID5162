
#ifndef _COM_CONST_H
#define _COM_CONST_H

#define TRUE               1	/* used for turning integers into Booleans */
#define FALSE              0	/* used for turning integers into Booleans */ 
#define BYTE_MASK          0377	/* mask for 8 bits */

#define TIMEOUT_NOWAIT			0	
#define TIMEOUT_FOREVER			(-1)

#define BITMAP_32BITS		32 

#ifdef DVK_GLOBAL_HERE
#define EXTERN	
#else
#define EXTERN	extern
#endif

#define PID_MAX	PID_MAX_LIMIT		
#define USER_ROOT	0

#define parm_vcopy_t vcopy_t


#endif // _COM_CONST_H





	

