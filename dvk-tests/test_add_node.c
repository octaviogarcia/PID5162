#include "tests.h"

void  main ( int argc, char *argv[] )
{
	int dcid, nodeid, ret;

	if ( argc != 3) {
		fprintf(stderr,  "Usage: %s <dcid> <nodeid>\n", argv[0] );
		exit(EXIT_FAILURE);
	}

	dcid = atoi(argv[1]);
	if ( dcid < 0 || dcid >= NR_DCS) {
		fprintf(stderr,  "Invalid dcid [0-%d]\n", NR_DCS-1 );
		exit(EXIT_FAILURE);
	}

	nodeid = atoi(argv[2]);
	if ( nodeid < 0 || nodeid >= NR_NODES) {
		fprintf(stderr, "Invalid nodeid [0-%d]\n", NR_NODES-1 );
		exit(EXIT_FAILURE);
	}
		
	ret = dvk_open();
	if (ret < 0)  ERROR_PRINT(ret);

    printf("Adding node %d to DC %d... \n", nodeid, dcid);

	ret = dvk_add_node(dcid, nodeid);
	if(ret) ERROR_PRINT(ret);
		
 }



