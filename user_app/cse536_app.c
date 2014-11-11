#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#define DEVICE_NAME	"/dev/cse536"
#define MAX_MSG_SIZE	256
#define ADDRESS_LEN	16


// Sets the destination address in teh character device
int  set_daddr(int fd, char *addr)
{
	int ret = 0;
	char *data = (char *)malloc(sizeof(char) * (strlen(addr) + 2));

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
	char *data = (char *)malloc(sizeof(char) * (strlen(msg) + 1));

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

	ret = read(fd, (void *)msg, MAX_MSG_SIZE);
	
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
	int choice;

	// Allocate memory for buffer
	data = (char *)malloc(sizeof(char)* MAX_MSG_SIZE);
	if(!data) {
		printf("%s: Insufficient memory\n", __FILE__);
		ret = -1;
		goto end;
	}

	// Open device file
	fd = open(DEVICE_NAME, O_RDWR);
	if(fd == -1) {
		printf("%s: Error opening %s : %s\n", __FILE__,	DEVICE_NAME, strerror(errno));
		ret = -1;
		goto free_data;
	}


	while(1) {

		print_choices();
		scanf("%d%c",&choice,&fix);
		fflush(stdin);

		switch(choice) {

			case 1:
				// Set the address
				printf("Enter the address: ");
				fgets(data, ADDRESS_LEN,stdin);
				if (set_daddr(fd, data) == -1) {
					printf("%s: Error setting the address\n", __FILE__);
					ret = -1;
					goto close_file;
				}	
				printf("Address set to %s successfully\n", data);
				break;

			case 2:
				// Send the data
				printf("Enter the message: ");
				fgets(data, MAX_MSG_SIZE, stdin);
				printf("%s: Sending message = %s\n",__FILE__, data);
				if (send_msg(fd, data) == -1) {
					printf("%s: Error sending the message\n",__FILE__);
					ret = -1;
					goto close_file;
				}
				break;

			case 3:
				// Read the data
				memset(data, 0, MAX_MSG_SIZE);
				if (recv_msg(fd, data) == -1) {
					printf("%s: Error receiving message\n",__FILE__);
					ret = -1;
					goto close_file;
				}
				printf("%s: Message received = %s\n",__FILE__, data);
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
free_data:
	if(data)
		free(data);
end:
	return ret;
}
