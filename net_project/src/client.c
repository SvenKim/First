
/*I'm client:TCP -----创建、绑定、连接、发送-------------------------------*/

#include "../include/myhead.h"

#define SIZE    150                             //缓冲区的大小
#define SERVER_PORT  8808                       //服务器的端口号
#define SERVER_IP  "192.168.90.8"               //服务器IP


int main()
{
	int ret;

	int socket_fd = socket(AF_INET,SOCK_STREAM,0);
	if(socket_fd == -1)
	{
		perror("socket() failed");
 		exit(0);
	}

	struct sockaddr_in server_addr;
        bzero(&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;		// TCP/IP协议
	server_addr.sin_port = htons(SERVER_PORT);	//初始化服务器IP及端口信息
	inet_pton(AF_INET,SERVER_IP,&server_addr.sin_addr);

	ret = connect(socket_fd,(struct sockaddr_in *)&server_addr,sizeof(server_addr));	//连接服务器
	if(ret == -1)
	{
		perror("connect failed");
		exit(0);
	}

	char *buf = malloc(SIZE);

	while(1)
	{
		bzero(buf,SIZE);
		
		if(fgets(buf,SIZE,stdin) == NULL)
			break;

		int n = send(socket_fd,buf,SIZE,0);
		if(n == -1)
		{
			perror("send failed!");
			exit(0);
		}
		printf("%d bytes date have been sent..\n",n);
	}
	
	close(socket_fd);


	return 0;
}
