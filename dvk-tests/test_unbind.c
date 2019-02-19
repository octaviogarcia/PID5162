#include "tests.h"
   
void  main ( int argc, char *argv[] )
{
	int dcid, pid, p_nr, ret, ep;

	if ( argc != 3) {
		fprintf(stderr,  "Usage: %s <dcid> <p_nr> \n", argv[0] );
		exit(EXIT_FAILURE);
	}

	dcid = atoi(argv[1]);
	if ( dcid < 0 || dcid >= NR_DCS) {
		fprintf(stderr,  "Invalid dcid [0-%d]\n", NR_DCS-1 );
		exit(EXIT_FAILURE);
	}

	p_nr = atoi(argv[2]);
	pid = getpid();
	
	ret = dvk_open();
	if (ret < 0)  ERROR_PRINT(ret);

    printf("Binding process %d to DC%d with p_nr=%d\n",pid,dcid,p_nr);
    ep = dvk_bind(dcid, p_nr);
	if( ep < EDVSERRCODE) ERROR_PRINT(ep);

	printf("waiting to end ep=%d errno=%d\n",ep, errno);
	sleep(10);
	ret = dvk_unbind(dcid,ep);
	printf("dvk_unbind ret=%d\n",ret);
 }



