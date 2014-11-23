#ifndef __CSE536_PROTOCOL_H__
#define __CSE536_PROTOCOL_H__

#define IPPROTO_CSE536	234
#define MAX_MSG_SIZE	257
#define RETRY_ATTEMPTS	2
#define WAIT_TIME_SEC	5

struct transaction_struct {
        uint32_t        recID;
        uint32_t        finalClock;
        uint32_t        originalClock;
        uint32_t        sourceAddr;
        uint32_t        destAddr;
        char            msg[236];
};

/* Structure for buffer linklist */
struct node {
        struct list_head list;
        struct transaction_struct *data;
};

int add_my_proto(void);
int del_my_proto(void);

int cse536_sendmsg(char *data, size_t len);
void cse536_getmsg(char *data, size_t *len);
int cse536_setaddr(char *addr);



#endif  /* cse536_protocol.h */
