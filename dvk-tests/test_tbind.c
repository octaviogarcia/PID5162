#include "tests.h"

#define    MAXTHREADS	2
int t_nr[MAXTHREADS];
int p_nr, dcid;

void *thread_bind(void *arg) {
	int ep , ret;
	pid_t tid, pid;

	tid = (pid_t) syscall (SYS_gettid);
	pid = getpid();
	printf("THREAD %d BIND Binding process %d to DC%d with p_nr=%d\n",tid,
		pid,dcid,p_nr);
	// PARENT BIND 	
	ep = dvk_bind(dcid, p_nr);
	if( ep < EDVSERRCODE) ERROR_PRINT(ep);
	// test dvk_getep	
	printf("THREAD %d BIND ep=%d dvk_getep=%d\n", 
		tid, ep, dvk_getep(pid));	
	sleep(60);
	printf("THREAD %d BIND exit\n", tid);
}

void *thread_tbind(void *arg) {
	int ep , ret;
	pid_t tid, pid;

	tid = (pid_t) syscall (SYS_gettid);
	pid = getpid();
	printf("THREAD %d TBIND Binding process %d to DC%d with p_nr+1=%d\n", tid,
		tid, dcid, p_nr+1);
	// SELF BIND 
	ep = dvk_tbind(dcid, p_nr+1);
	if( ep < EDVSERRCODE) ERROR_PRINT(ep);
	// test dvk_getep	
	printf("THREAD %d TBIND ep=%d dvk_getep=%d\n", tid, ep, 
		dvk_getep(tid));	
	sleep(60);
	printf("THREAD %d TBIND exit\n", tid);
}

void  main ( int argc, char *argv[] )
{
	int i, pid, ret, ep, tid; 
   	pthread_t mythread[MAXTHREADS];
	int thr_idx = 0;

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
	
	ret = dvk_open();
	if (ret < 0)  ERROR_PRINT(ret);

	pid = getpid();
	tid = (pid_t) syscall (SYS_gettid);
	thr_idx = 0;

	printf("PARENT %d starting thread %d\n", pid, thr_idx);
	// FIRST THREAD TRY TO BIND MAIN THREAD
	if ( (ret = pthread_create(&mythread[thr_idx], NULL, thread_bind, thr_idx))) {
		ERROR_PRINT(ret);
		exit(1);
	}
	sleep(5);
	printf("PARENT pid=%d tid=%d BIND ep=%d dvk_getep=%d\n", 
		pid, tid, ep, dvk_getep(pid));

	// SECOND THREAD TRY TO BIND ITSELF
	thr_idx++;
	printf("PARENT %d starting thread %d\n", pid, thr_idx);
	if ( (ret = pthread_create(&mythread[thr_idx], NULL, thread_tbind, thr_idx))) {
		ERROR_PRINT(ret);
		exit(1);
	}
	
	for( i = 0; i < thr_idx; i++) {
		printf("PARENT %d waiting children i=%d\n", pid, i);
		if ( ret = pthread_join ( mythread[i], NULL ) ) {
			ERROR_PRINT(ret);
			exit(1);
		}
	}
	
	if(ret) ERROR_PRINT(ret);
	printf("PARENT %d exiting\n", pid);
	
 }



