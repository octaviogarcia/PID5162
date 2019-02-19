/* Function prototypes for the DVSD 
 */ 

#ifndef TABLE_H
#define TABLE_H


/* Default handler for unused kernel calls. */
_PROTOTYPE( int do_unused, (dvs_cmd_t *dvscmd_ptr) );

_PROTOTYPE( int do_help, (dvs_cmd_t *dvscmd_ptr) );

_PROTOTYPE( int do_dvsinit, (dvs_cmd_t *dvscmd_ptr) );		
#if ! USE_DVSINIT
#define do_dvsinit do_unused
#endif

_PROTOTYPE( int do_dvsend, (dvs_cmd_t *dvscmd_ptr) );		
#if ! USE_DVSEND
#define do_dvsend do_unused
#endif

_PROTOTYPE( int do_dvsinfo, (dvs_cmd_t *dvscmd_ptr) );		
#if ! USE_DVSINFO
#define do_dvsinfo do_unused
#endif

_PROTOTYPE( int do_dcinfo, (dvs_cmd_t *dvscmd_ptr) );		
#if ! USE_DCINFO
#define do_dcinfo do_unused
#endif

_PROTOTYPE( int do_nodeinfo, (dvs_cmd_t *dvscmd_ptr) );		
#if ! USE_NODEINFO
#define do_nodeinfo do_unused
#endif

_PROTOTYPE( int do_proxyinfo, (dvs_cmd_t *dvscmd_ptr) );		
#if ! USE_PROXYINFO
#define do_proxyinfo do_unused
#endif

_PROTOTYPE( int do_procinfo, (dvs_cmd_t *dvscmd_ptr) );		
#if ! USE_PROCINFO
#define do_procinfo do_unused
#endif

#endif	/* TABLE_H */

