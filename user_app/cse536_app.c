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
#define IPPROTO_CSE536	234

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


int send_msg(int fd, char *msg)
{
	int ret = 0;
	char *data = (char *)malloc(sizeof(char) * (strlen(msg) + 1));

	data[0] = '0';
	strcpy(data+1, msg);

	ret = write(fd, (void *)data, strlen(data));

	if(data)
		free(data);

	return ret > 0 ? 1 : -1;
}

int recv_msg(int fd, char *msg)
{
	int ret = 0;

	ret = read(fd, (void *)msg, MAX_MSG_SIZE);
	
	return ret >= 0 ? 1 : -1;
}

int main(int argc, char **argv)
{
	int fd;
	int ret = 0;
	char *data = NULL;

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

	// Set the address
	if (set_daddr(fd, "127.0.0.1") == -1) {
		printf("%s: Error setting the address\n", __FILE__);
		ret = -1;
		goto close_file;
	}	

	// Send the data
	strcpy(data, "Call of Duty: Advanced Warfare");
	if (send_msg(fd, data) == -1) {
		printf("%s: Error sending the message\n",__FILE__);
		ret = -1;
		goto close_file;
	}


	// Read the data
	if (recv_msg(fd, data) == -1) {
		printf("%s: Error receiving message\n",__FILE__);
		ret = -1;
		goto close_file;
	}
	printf("%s: Message received = %s\n",__FILE__, data);



close_file:
	close (fd);
free_data:
	if(data)
		free(data);
end:
	return ret;
}
