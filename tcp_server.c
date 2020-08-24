#include "tcp.h"

void sentFile(int sockfd)
{
	char file_name[MAX], buff[MAX]; // for read operation from file and used to sent operation
	int send_fd, read_len;
	int exist, ret;

	memset(buff, 0x00, MAX);

	read_len = read(sockfd, buff, MAX);
	if (read_len == 0)
	{
		return;
	}

	strcpy(file_name, buff);
	printf("File requested > %s\n", file_name);

	if (access(file_name, F_OK) < 0)
	{
		exist = -1;
	}

	ret = send(sockfd, &exist, sizeof(exist), 0);

	if (ret < 0)
	{
		printf("Failed to make aware(send) client if the file exists or not\n");
	}

	if (exist < 0)
	{
		printf("%s doesn't exist in server\n", file_name);
		return;
	}

	FILE* fp;
	fp = fopen(file_name,"rb");
	fseek(fp, 0, SEEK_END);
	long long int fsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	char* fbuff = (char*)malloc(fsize);

	if (fbuff==NULL){
		printf("Memory not allocated for the file buffer");
		return;
	}

	for(;;){
		size_t bytesread = fread(fbuff, 1, fsize, fp);
		ret = send(sockfd, fbuff, bytesread, 0);

		if (ret<0)
		{
			printf("Failed to send the file buffer");
		}
		
		if (bytesread==0){
			printf("File copied to the buffer and sent to the client");
			realloc(fbuff, 0);
			break;
		}
	}
}

int main()
{
	int sockfd, connfd, len;		  // create socket file descriptor
	struct sockaddr_in servaddr, cli; // create structure object of sockaddr_in for client and server

	// socket create and verification
	sockfd = socket(AF_INET, SOCK_STREAM, 0); // creating a TCP socket ( SOCK_STREAM )

	if (sockfd == -1)
	{
		printf("socket creation failed...\n");
		exit(0);
	}
	else
		printf("Socket successfully created..\n");

	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
		printf("setsockopt(SO_REUSEADDR) failed");

	// empty the
	bzero(&servaddr, sizeof(servaddr));

	// assign IP, PORT
	servaddr.sin_family = AF_INET;				  // specifies address family with IPv4 Protocol
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // binds to any address
	servaddr.sin_port = htons(PORT);			  // binds to PORT specified

	// Binding newly created socket to given IP and verification
	if ((bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
	{
		printf("socket bind failed...\n");
		exit(0);
	}
	else
		printf("Socket successfully binded..\n");

	// Now server is ready to listen and verification
	if ((listen(sockfd, 5)) != 0)
	{
		printf("Listen failed...\n");
		exit(0);
	}
	else
		printf("Server listening..\n");

	len = sizeof(cli);

	// Accept the data packet from client and verification
	connfd = accept(sockfd, (struct sockaddr *)&cli, &len); // accepts connection from socket

	if (connfd < 0)
	{
		printf("server acccept failed...\n");
		exit(0);
	}
	else
		printf("server acccept the client...\n");

	// Function for chatting between client and server
	sentFile(connfd);

	// After transfer close the socket
	close(sockfd);
}