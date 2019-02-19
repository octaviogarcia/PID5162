
// The maximum payload size for UDP over IPv4 is 65507 bytes
// However, the largest payload that an implementation is required to support is 548 bytes for IPv4
// 1472 bytes would be a reasonable choice. 
// we set it in 1024 
#define MAX_MESSLEN  		sizeof((struct cmsghdr *)) + 1024
char msgbuf[MAX_MESSLEN];

