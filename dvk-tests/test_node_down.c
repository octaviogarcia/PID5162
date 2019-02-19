#include "tests.h"

/*----------------------------------------------*/
/*		MAIN: 			*/
/*----------------------------------------------*/
void  main ( int argc, char *argv[] )
{
	int ret,  nodeid, pxnr;

    	if ( argc != 2) {
 	        printf( "Usage: %s <nodeid>\n", argv[0] );
 	        exit(1);
	    }

	nodeid = atoi(argv[1]);

	printf("nodeid=%d \n", nodeid);

	ret = dvk_open();
	if (ret < 0)  ERROR_PRINT(ret);
	
	/* deregister the node */
	ret = dvk_node_down(nodeid);
	if( ret) ERROR_PRINT(ret);
	
	exit(0);
 }
