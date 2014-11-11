#ifndef __CSE536_PROTOCOL_H__
#define __CSE536_PROTOCOL_H__

#define IPPROTO_CSE536	234
#define MAX_MSG_SIZE	256

// destination and local address variables
extern __be32 cse536_daddr;
extern __be32 cse536_saddr;
extern const struct net_protocol cse536_protocol;



static int cse536_recv(struct sk_buff *skb);
static void cse536_error(struct sk_buff *skb, u32 info);
static int add_my_proto(void);
static int del_my_proto(void);

static int cse536_sendmsg(char *data, size_t len);
static void getLocalAddress(void);



#endif  /* cse536_protocol.h */
