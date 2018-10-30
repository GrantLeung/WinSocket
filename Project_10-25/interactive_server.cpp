#pragma once
#define HAVE_STRUCT_TIMESPEC
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>
#pragma warning(disable:4996)


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
extern "C" {
#include "ladder.h"
}

uint8_t run_openplc = 1;


//Global Variables
bool run_modbus = 0;
int modbus_port = 502;
bool run_dnp3 = 0;
int dnp3_port = 20000;
unsigned char server_command[1024];
int command_index = 0;
bool processing_command = 0;
time_t start_time;
time_t end_time;

//Global Threads
pthread_t modbus_thread;
pthread_t dnp3_thread;


int readCommandArgument(char *command)
{
	int i = 0;
	int j = 0;
	char argument[1024];

	while (command[i] != '(' && command[i] != '\0') i++;
	if (command[i] == '(')i++;
	while (command[i] != ')' && command[i] != '\0')
	{
		argument[j] = command[i];
		i++;
		j++;
		argument[j] = '\0';
	}

	return atoi(argument);
}

int createSocket_interactive(int port)
{
	unsigned char log_msg[1000];
	int socket_fd;
	struct sockaddr_in server_addr;

	//Initial the Socket
	WSADATA wsd;
	if (WSAStartup(MAKEWORD(2, 2), &wsd) != 0) {
		sprintf((char*)log_msg, "Winsock ≥ı ºªØ ß∞‹!\n");
		log(log_msg);
		return 1;
	}

	//Create TCP Socket
	socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (socket_fd < 0)
	{
		sprintf((char*)log_msg, "Interactive Server: error creating stream socket => %s\n", strerror(errno));
		log(log_msg);
		exit(1);
	}

	int enable = 1;
	if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&enable, sizeof(int)) < 0)
		perror("setsockopt(SO_REUSEADDR) failed");

	SetSocketBlockingEnabled(socket_fd, false);

	//Initialize Server Struct
	memset((char *)&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	server_addr.sin_port = htons(port);

	//Bind socket
	if (bind(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		sprintf((char*)log_msg, "Interactive Server: error binding socket => %s\n", strerror(errno));
		log(log_msg);
		exit(1);
	}

	//Accept max 5 pending connections
	listen(socket_fd, 5);
	sprintf((char*)log_msg, "Interactive Server: Listening on port %d\n", port);
	log(log_msg);
	return socket_fd;
}

int waitForClient_interactive(int socket_fd)
{
	int client_fd = -1;
	struct sockaddr_in client_addr;
	socklen_t client_len;

	printf("Interactive Server: waiting for new client...\n");

	client_len = sizeof(client_addr);

	while (run_openplc)
	{
		client_fd = accept(socket_fd, (sockaddr *)&client_addr, (socklen_t*)&client_len); //non-blocking call
		if (client_fd > 0)
		{
			SetSocketBlockingEnabled(client_fd, true);
			break;
		}
		Sleep(100);
	}

	return client_fd;
}

int listenToClient_interactive(int client_fd, char *buffer)
{
	memset(buffer, 0, 1024);
	int n = recv(client_fd, buffer, 1024, 0);
	return n;
}

void processCommand(char *buffer, int client_fd)
{
	unsigned char log_msg[1000];
	int count_char = 0;

	if (processing_command)
	{
		count_char = sprintf(buffer, "Processing command...\n");
		send(client_fd, buffer, count_char, 0);
		return;
	}

	if (strncmp(buffer, "quit()", 6) == 0)
	{
		processing_command = true;
		sprintf((char*)log_msg, "Issued quit() command\n");
		log(log_msg);
		if (run_modbus)
		{
			run_modbus = 0;
			pthread_join(modbus_thread, NULL);
			sprintf((char*)log_msg, "Modbus server was stopped\n");
			log(log_msg);
		}
		if (run_dnp3)
		{
			run_dnp3 = 0;
			pthread_join(dnp3_thread, NULL);
			sprintf((char*)log_msg, "DNP3 server was stopped\n");
			log(log_msg);
		}
		run_openplc = 0;
		processing_command = false;
	}
	//else if (strncmp(buffer, "start_modbus(", 13) == 0)
	//{
	//	processing_command = true;
	//	sprintf(log_msg, "Issued start_modbus() command to start on port: %d\n", readCommandArgument(buffer));
	//	log(log_msg);
	//	modbus_port = readCommandArgument(buffer);
	//	if (run_modbus)
	//	{
	//		sprintf(log_msg, "Modbus server already active. Restarting on port: %d\n", modbus_port);
	//		log(log_msg);
	//		//Stop Modbus server
	//		run_modbus = 0;
	//		pthread_join(modbus_thread, NULL);
	//		sprintf(log_msg, "Modbus server was stopped\n");
	//		log(log_msg);
	//	}
	//	//Start Modbus server
	//	run_modbus = 1;
	//	pthread_create(&modbus_thread, NULL, modbusThread, NULL);
	//	processing_command = false;
	//}
	else if (strncmp(buffer, "stop_modbus()", 13) == 0)
	{
		processing_command = true;
		sprintf((char*)log_msg, "Issued stop_modbus() command\n");
		log(log_msg);
		if (run_modbus)
		{
			run_modbus = 0;
			pthread_join(modbus_thread, NULL);
			sprintf((char*)log_msg, "Modbus server was stopped\n");
			log(log_msg);
		}
		processing_command = false;
	}
	//else if (strncmp(buffer, "start_dnp3(", 11) == 0)
	//{
	//	processing_command = true;
	//	sprintf(log_msg, "Issued start_dnp3() command to start on port: %d\n", readCommandArgument(buffer));
	//	log(log_msg);
	//	dnp3_port = readCommandArgument(buffer);
	//	if (run_dnp3)
	//	{
	//		sprintf(log_msg, "DNP3 server already active. Restarting on port: %d\n", dnp3_port);
	//		log(log_msg);
	//		//Stop DNP3 server
	//		run_dnp3 = 0;
	//		pthread_join(dnp3_thread, NULL);
	//		sprintf(log_msg, "DNP3 server was stopped\n");
	//		log(log_msg);
	//	}
	//	//Start DNP3 server
	//	run_dnp3 = 1;
	//	pthread_create(&dnp3_thread, NULL, dnp3Thread, NULL);
	//	processing_command = false;
	//}
	else if (strncmp(buffer, "stop_dnp3()", 11) == 0)
	{
		processing_command = true;
		sprintf((char*)log_msg, "Issued stop_dnp3() command\n");
		log(log_msg);
		if (run_dnp3)
		{
			run_dnp3 = 0;
			pthread_join(dnp3_thread, NULL);
			sprintf((char*)log_msg, "DNP3 server was stopped\n");
			log(log_msg);
		}
		processing_command = false;
	}
	else if (strncmp(buffer, "runtime_logs()", 14) == 0)
	{
		processing_command = true;
		printf("Issued runtime_logs() command\n");
		send(client_fd, log_buffer, log_index, 0);
		processing_command = false;
		return;
	}
	else if (strncmp(buffer, "exec_time()", 11) == 0)
	{
		processing_command = true;
		time(&end_time);
		count_char = sprintf(buffer, "%llu\n", (unsigned long long)difftime(end_time, start_time));
		send(client_fd, buffer, count_char, 0);
		processing_command = false;
		return;
	}
	else
	{
		processing_command = true;
		count_char = sprintf(buffer, "Error: unrecognized command\n");
		send(client_fd, buffer, count_char, 0);
		processing_command = false;
		return;
	}

	count_char = sprintf(buffer, "OK\n");
	send(client_fd, buffer, count_char, 0);
}

void *handleConnections_interactive(void *arguments)
{
	unsigned char log_msg[1024];
	int client_fd = *(int *)arguments;
	char buffer[1024];
	int messageSize;

	printf("Interactive Server: Thread created for client ID: %d\n", client_fd);

	while (run_openplc)
	{
		messageSize = listenToClient_interactive(client_fd, buffer);
		if (messageSize <= 0 || messageSize > 1024)
		{
			if (messageSize == 0)
				printf("Interactive Server: client ID: %d has closed the connection\n", client_fd);
			else
				printf("Interactive Server: Something is wrong with the  client ID: %d message Size : %d\n", client_fd, messageSize);
			break;
		}
		sprintf((char*)log_msg, "Interactive Server: client has connected! \n");
		log(log_msg);
	}

	closeSocket(client_fd);
	printf("Terminating interactive server connections\n");
	pthread_exit(NULL);
	return NULL;
}

void startInteractiveServer(int port)
{
	unsigned char log_msg[1000];
	int socket_fd, client_fd;
	socket_fd = createSocket_interactive(port);
	while (run_openplc)
	{
		client_fd = waitForClient_interactive(socket_fd);
		if (client_fd < 0)
		{
			sprintf((char*)log_msg, "Interactive Server: Error accepting client!\n");
			log(log_msg);
		}

		else
		{
			int arguments[1];
			pthread_t thread;
			int ret = -1;

			printf("Interactive Server: Client accepted! Creating thread for the new client ID: %d...\n", client_fd);
			arguments[0] = client_fd;
			ret = pthread_create(&thread, NULL, handleConnections_interactive, arguments);
			if (ret == 0)
			{
				pthread_detach(thread);
			}
		}
	}
	printf("Closing socket...");
	closeSocket(socket_fd);
	closeSocket(client_fd);
	WSACleanup();
	sprintf((char*)log_msg, "Terminating interactive server thread \n");
	log(log_msg);
}
