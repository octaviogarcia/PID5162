#include "tests.h"

 

/*----------------------------------------------*/
/*		MAIN: 			*/
/*----------------------------------------------*/
void  main ( int argc, char *argv[] )
{
	int ret,  nodeid, pxnr;

    if ( argc != 4) {
 	    printf( "Usage: %s <node_name> <nodeid> <pxnr>\n", argv[0] );
 	    exit(1);
	}

	nodeid = atoi(argv[2]);
	pxnr = atoi(argv[3]);
	printf("node_name=%s nodeid=%d pxnr=%d \n",argv[1], nodeid, pxnr);
	/* register the proxies */
	
	ret = dvk_open();
	if (ret < 0)  ERROR_PRINT(ret);
	
	ret = dvk_node_up(argv[1], nodeid, pxnr);
	if( ret) ERROR_PRINT(ret);
	
	exit(0);
 }
