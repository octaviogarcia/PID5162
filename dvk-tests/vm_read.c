#include "tests.h"


#define BUFSIZE 100

int main(void)
{
    struct iovec parent_v[1];
    struct iovec child_v[1];

    ssize_t nread;
    pid_t child_pid;             /* PID of child process */
    pid_t parent_pid;             /* PID of parent process */
	char *parent_buf;
	char *child_buf;

	parent_buf = (char *) malloc(BUFSIZE);
	memset(parent_buf,'P',BUFSIZE-1);
	printf("BEFORE parent_buf=%s \n", parent_buf);
                  

	if( child_pid = fork()) { // PARENT
		sleep(10);
		printf("AFTER parent_buf=%s \n", parent_buf);
	}else{ // CHILD
		child_buf = (char *) malloc(BUFSIZE);
		memset(child_buf,'C',BUFSIZE-1);
		printf("BEFORE child_buf=%s \n", child_buf);
	
	    parent_v[0].iov_base = parent_buf;
		parent_v[0].iov_len = BUFSIZE;
		child_v[0].iov_base = child_buf;
		child_v[0].iov_len = BUFSIZE;
		parent_pid = getppid();
	    nread = process_vm_readv(parent_pid, child_v, 1, parent_v, 1, 0);
  		printf("CHILD AFTER child_buf=%s nread=%d errno=%d\n", 
			child_buf, nread, errno);	
		exit(0);
	}
	wait(0);
}