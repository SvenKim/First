
/*I'm server:TCP-----------创建、绑定、监听、等待连接、接收------------------*/

#include "../include/myhead.h"

#define SIZE 	150				//缓冲区的大小
#define SERVER_PORT  8808			//服务器的端口号
#define SERVER_IP  "10.10.10.114"		//服务器IP
#define LISTEN_LENTH 10				//监听最大请求数


int main()
{
	int ret;

	int socket_fd = socket(AF_INET,SOCK_STREAM,0);	//创建套接字
	if(socket_fd == -1)
	{
		perror("create socket failed");
		exit(0);	
	}

	struct sockaddr_in server_addr;			
	bzero(&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;			// TCP/IP协议
	server_addr.sin_port = htons(SERVER_PORT);		//初始化服务器IP及端口信息
	inet_pton(AF_INET,SERVER_IP,&server_addr.sin_addr);	
	
	ret = bind(socket_fd,(struct sockaddr_in *)&server_addr,sizeof(server_addr));	//绑定端口和IP
	if(ret == -1)
	{
		perror("bind IP+PORT failed");
		exit(0);
	}

	ret = listen(socket_fd,LISTEN_LENTH);			//监听
	if(ret == -1)
        {
        	perror("listen failed");
               	exit(0);
        }

	int client_fd = accept(socket_fd,NULL,NULL);		//等待客户端连接
	if(client_fd == -1)
	{
		perror("accept failed");
		exit(0);
	}
	
	char *buf = malloc(SIZE);				//定义一块缓冲区，大小为SIZE
	while(1)
	{
		bzero(buf,SIZE);
		if(recv(client_fd,buf,SIZE,0) == 0)		//接收数据
			break;
		
		printf("the message:%s\n",buf);
	}


	return 0;
}
