#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define DEVICE_NAME	"/dev/cse536"
#define MAX_MSG_SIZE	236
#define ADDRESS_LEN	17
#define SERVER_PORT 	23456 
#define MAX_PENDING 	5
#define MAX_LINE 	256

// Every message should be in this format
struct transaction_struct {
        uint32_t        recID;
        uint32_t        finalClock;
        uint32_t        originalClock;
        uint32_t        sourceAddr;
        uint32_t        destAddr;
        char            msg[MAX_MSG_SIZE];
};

// Prints choices for the user
void print_choices()
{
	printf("------------------------------\n");
	printf("1 --- Set destination address \n");
	printf("  --- and Send message	      \n");
	printf("3 --- Read Message	      \n");
	printf("4 --- Exit		      \n");
	printf("------------------------------\n");
	printf("Enter your choice: ");
}


// Main code
int main(int argc, char **argv)
{
	int fd;
	int ret = 0;
	int choice;
	int len, n, s, new_s;
	char fix;
	char *data = NULL;
	struct transaction_struct *buff = NULL;
	struct in_addr netaddr;
	struct sockaddr_in client, server;
	struct hostent *hp;


	// initialize the network variables
	bzero((char *)&server, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(0);

	// Allocate memory for input buffer
	data = (char *)malloc(sizeof(char)* MAX_MSG_SIZE);
	if(!data) {
		printf("%s: Insufficient memory\n", __FILE__);
		ret = -1;
		goto end;
	}

	// Allocate memeory for transaction buffer
	buff = (struct transaction_struct *)malloc(sizeof(struct transaction_struct));
	if (!buff) {
		printf("%s: Insufficient memory\n", __FILE__);
		ret = -1;
		goto free_data;
	}

	// Open device file
	fd = open(DEVICE_NAME, O_RDWR);
	if(fd == -1) {
		printf("%s: Error opening %s : %s\n", __FILE__,	DEVICE_NAME, strerror(errno));
		ret = -1;
		goto free_buff;
	}

	// Connecting to the UDP server
	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0)
	{
		perror("simplex-talk: UDP_socket error");
		exit(1);
	}

	if ((bind(s, (struct sockaddr *)&server, sizeof(server))) < 0)
	{
		perror("simplex-talk: UDP_bind error");
		exit(1);
	}

	hp = gethostbyname( "192.168.0.4" );
	if( !hp )
	{
		fprintf(stderr, "Unknown host %s\n", "localhost");
		exit(1);
	}

	bzero( (char *)&server, sizeof(server));
	server.sin_family = AF_INET;
	bcopy( hp->h_addr, (char *)&server.sin_addr, hp->h_length );
	server.sin_port = htons(SERVER_PORT); 



	while(1) {

		print_choices();
		scanf("%d%c",&choice,&fix);
		fflush(stdin);

		switch(choice) {

			case 1:
				// Set the address
				memset(data, 0, MAX_MSG_SIZE);
				printf("Enter the address: ");
				fgets(data, ADDRESS_LEN, stdin);
				data[strlen(data)-1] = '\0';
				if(!inet_aton(data, &netaddr)){
					printf("%s: Invalid address\n", __FILE__);
					continue;
				}
 				buff->destAddr = netaddr.s_addr;

				// Send the data
				memset(data, 0, MAX_MSG_SIZE);
				printf("Enter the message: ");
				fgets(data, MAX_MSG_SIZE, stdin);
				data[strlen(data)-1] = '\0';
				strncpy(buff->msg, data, MAX_MSG_SIZE);
				buff->recID = 1;
				buff->originalClock = buff->finalClock = 0;
				if (write(fd, (char *)buff, sizeof(struct transaction_struct)) == -1) {
					printf("%s: Error sending the message\n",__FILE__);
				}
				//
				// Send the evnet struct to the udp server
				ret = sendto(s, (char *)buff, MAX_LINE, 0,(struct sockaddr *)&server, sizeof(server));
				break;

			case 3:
				// Read the data
				memset(buff, 0, sizeof(struct transaction_struct));
				if ( read(fd, (char *)buff, sizeof(struct transaction_struct)) == -1) {
					printf("%s: Error receiving message\n",__FILE__);
					ret = -1;
					goto close_file;
				}
				 
				// Send this event to udp server
				ret = sendto(s, (char *)buff, MAX_LINE, 0,(struct sockaddr *)&server, sizeof(server));
				break;

			case 4:
				ret = 0;
				goto close_file;
			default:
				printf("Invalid choice\n");
		}

	}

		
close_file:
	close (fd);
free_buff:
	if(buff)
		free(buff);
free_data:
	if(data)
		free(data);
end:
	return ret;
}
