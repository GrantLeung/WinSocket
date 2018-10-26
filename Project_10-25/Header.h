#pragma once
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
//#include <fstream>
#define HAVE_STRUCT_TIMESPEC
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable:4996)
#include <time.h>

#include <ws2tcpip.h>

#include <winsock2.h>
#pragma comment(lib,"ws2_32.lib")

#include <string.h>
#include <signal.h>
#include <synchapi.h>

#include <pthread.h>
#pragma comment(lib, "x86/pthreadVC2.lib")

#include <Windows.h>
#pragma comment(lib, "wsock32.lib")

#include <Mmsystem.h>
#pragma comment(lib, "Winmm.lib")
pthread_mutex_t logLock; //mutex for the internal log
pthread_mutex_t bufferLock; //mutex for the internal buffers
pthread_mutex_t pid_lock;
pthread_mutex_t pid_lock2;
uint8_t run_openplc = 1;
char log_buffer[1000000]; //A very large buffer to store all logs
int log_index = 0;
int log_counter = 0;

void log(char *logmsg)
{
	pthread_mutex_init(&logLock, NULL);
	pthread_mutex_lock(&logLock); //lock mutex
	printf("%s", logmsg);
	for (int i = 0; logmsg[i] != '\0'; i++)
	{
		log_buffer[log_index] = logmsg[i];
		log_index++;
		log_buffer[log_index] = '\0';
	}

	log_counter++;
	if (log_counter >= 1000)
	{
		/*Store current log on a file*/
		log_counter = 0;
		log_index = 0;
	}
	pthread_mutex_unlock(&logLock); //unlock mutex
}

int getSO_ERROR(int fd)
{
	int err = 1;
	socklen_t len = sizeof err;
	if (-1 == getsockopt(fd, SOL_SOCKET, SO_ERROR, (char *)&err, &len))
		perror("getSO_ERROR");
	if (err)
		errno = err;              // set errno to the socket SO_ERROR
	return err;
}

void closeSocket(int fd)
{
	if (fd >= 0)
	{
		getSO_ERROR(fd);
		if (shutdown(fd, SD_BOTH) < 0) // secondly, terminate the 'reliable' delivery
			if (errno != WSAENOTCONN  && errno != EINVAL) // SGI causes EINVAL
				perror("shutdown");
		if (closesocket(fd) < 0) // finally call close()
			perror("close");
	}
}

int createSocket_interactive(int port)
{
	char log_msg[1000];
	int socket_fd;
	struct sockaddr_in server_addr;

	//Initial the Socket
	WSADATA wsd;
	if (WSAStartup(MAKEWORD(2, 2), &wsd) != 0) {
		sprintf(log_msg, "Winsock ³õÊ¼»¯Ê§°Ü!\n");
		log(log_msg);
		return 1;
	}

	//Create TCP Socket
	socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (socket_fd < 0)
	{
		sprintf(log_msg, "Interactive Server: error creating stream socket => %s\n", strerror(errno));
		log(log_msg);
		exit(1);
	}

	int enable = 1;
	if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&enable, sizeof(int)) < 0)
		perror("setsockopt(SO_REUSEADDR) failed");

	//SetSocketBlockingEnabled(socket_fd, false);

	//Initialize Server Struct
	memset((char *)&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	server_addr.sin_port = htons(port);

	//Bind socket
	if (bind(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		sprintf(log_msg, "Interactive Server: error binding socket => %s\n", strerror(errno));
		log(log_msg);
		exit(1);
	}

	//Accept max 5 pending connections
	listen(socket_fd, 5);
	sprintf(log_msg, "Interactive Server: Listening on port %d\n", port);
	log(log_msg);
	return socket_fd;
}

int waitForClient_interactive(int socket_fd)
{
	int client_fd;
	struct sockaddr_in client_addr;
	socklen_t client_len;

	printf("Interactive Server: waiting for new client...\n");

	client_len = sizeof(client_addr);

	while (run_openplc)
	{
		client_fd = accept(socket_fd, (sockaddr *)&client_addr, &client_len); //non-blocking call
		if (client_fd > 0)
		{
			//SetSocketBlockingEnabled(client_fd, true);
			char log_msg[1000];
			sprintf(log_msg, "Connect client successfully!");
			log(log_msg);
			break;
		}
		Sleep(1000);
	}

	return client_fd;
}

int listenToClient_interactive(int client_fd, char *buffer)
{
	memset(buffer, 0 , 1024);
	int n = recv(client_fd, buffer, 1024,0);
	return n;

}


