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

#define DEVICE_NAME	"/dev/cse536"
#define MAX_MSG_SIZE	236
#define ADDRESS_LEN	16

// Every message should be in this format
struct transaction_struct {
        uint32_t        recID;
        uint32_t        finalClock;
        uint32_t        originalClock;
        uint32_t        sourceAddr;
        uint32_t        destAddr;
        char            msg[MAX_MSG_SIZE];
};


// Sets the destination address in teh character device
int  set_daddr(int fd, char *addr)
{
	int ret = 0;
	char *data = (char *)malloc(sizeof(struct transaction_struct)  + 1);

	data[0] = '1';
	strcpy(data+1, addr);

	ret = write(fd, (void *)data, strlen(data));
	
	if(data)
		free(data);

	return ret > 0 ? 1 : -1;
}


// Sends the message to  the character device
int send_msg(int fd, char *msg)
{
	int ret = 0;
	char *data = (char *)malloc(sizeof(struct transaction_struct) + 1);

	data[0] = '2';
	strcpy(data+1, msg);

	ret = write(fd, (void *)data, strlen(data));

	if(data)
		free(data);

	return ret > 0 ? 1 : -1;
}


// Gets the message from teh buffer in the device
int recv_msg(int fd, char *msg)
{
	int ret = 0;

	ret = read(fd, (void *)msg, sizeof(struct transaction_struct));
	
	return ret >= 0 ? 1 : -1;
}


// Prints choices for the user
void print_choices()
{
	printf("------------------------------\n");
	printf("1 --- Set destination address \n");
	printf("2 --- Send message	      \n");
	printf("3 --- Read Message	      \n");
	printf("4 --- Exit		      \n");
	printf("------------------------------\n");
	printf("Enter your choice: ");
}


// Main code
int main(int argc, char **argv)
{
	int fd;
	char fix;
	int ret = 0;
	char *data = NULL;
	struct transaction_struct *buff = NULL;
	int choice;

	// Allocate memory for input buffer
	data = (char *)malloc(sizeof(char)* MAX_MSG_SIZE);
	if(!data) {
		printf("%s: Insufficient memory\n", __FILE__);
		ret = -1;
		goto end;
	}

	// Allocate memeory for transaction buffer
	buff = (struct transcation_struct *)malloc(sizeof(struct transaction_struct));
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


	while(1) {

		print_choices();
		scanf("%d%c",&choice,&fix);
		fflush(stdin);

		switch(choice) {

			case 1:
				// Set the address
				printf("Enter the address: ");
				fgets(data, ADDRESS_LEN, stdin);
				buff->destAddr = inet_aton(data);	
				if (set_daddr(fd, (char *)buff) == -1) {
					printf("%s: Error setting the address\n", __FILE__);
					ret = -1;
					goto close_file;
				}	
				printf("Address set to %s successfully\n", data);
		
			case 2:
				// Send the data
				printf("Enter the message: ");
				fgets(data, MAX_MSG_SIZE, stdin);
				strncpy(buff->msg, data, MAX_MSG_SIZE);
				printf("%s: Sending message = %s\n",__FILE__, data);
				if (send_msg(fd, (char *)buff) == -1) {
					printf("%s: Error sending the message\n",__FILE__);
					ret = -1;
					goto close_file;
				}
				break;

			case 3:
				// Read the data
				memset(data, 0, MAX_MSG_SIZE);
				if (recv_msg(fd, (char *)buff) == -1) {
					printf("%s: Error receiving message\n",__FILE__);
					ret = -1;
					goto close_file;
				}
				printf("%s: Message received = %s\n",__FILE__, buff->msg);
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
