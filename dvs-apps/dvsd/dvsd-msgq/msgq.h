

#define MAX_MESSLEN  (sizeof(dvs_cmd_t)+ (64 * 1024))

struct msgbuf_s {
    long mtype;       /* message type, must be > 0 */
    char mtext[MAX_MESSLEN];    /* message data */
};

