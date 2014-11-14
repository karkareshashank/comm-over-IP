#ifndef __CSE536_PROTOCOL_H__
#define __CSE536_PROTOCOL_H__

#define IPPROTO_CSE536	234
#define MAX_MSG_SIZE	256

/* Structure for buffer linklist */
struct node {
        struct list_head list;
        char    *data;
	unsigned int len;
};

int add_my_proto(void);
int del_my_proto(void);

int cse536_sendmsg(char *data, size_t len);
void cse536_getmsg(char *data, size_t *len);
void cse536_setaddr(char *data);



#endif  /* cse536_protocol.h */
