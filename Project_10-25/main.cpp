#include <stdio.h>
#include "Header.h"

void *handleConnections_interactive(void *arguments)
{
	char log_msg[1024];
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
				printf("Interactive Server: Something is wrong with the  client ID: %d message Size : %i\n", client_fd, messageSize);
			break;
		}
		sprintf(log_msg, "Interactive Server: client has connected! \n");
		log(log_msg);
	}

	closeSocket(client_fd);
	printf("Terminating interactive server connections\r\n");
	pthread_exit(NULL);
	return NULL;
}

void startInteractiveServer(int port)
{
	char log_msg[1000];
	int socket_fd, client_fd;
	socket_fd = createSocket_interactive(port);
	while (run_openplc)
	{
		client_fd = waitForClient_interactive(socket_fd);
		if (client_fd < 0)
		{
			sprintf(log_msg, "Interactive Server: Error accepting client!\n");
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
	sprintf(log_msg, "Terminating interactive server thread \n");
	log(log_msg);
}

void* interactiveServerThread(void*)
{
	startInteractiveServer(43628);
	
	return NULL;
}

int main()
{
	char log_msg[1000];
	sprintf(log_msg, "OpenPLC Runtime starting...\n");

	log(log_msg);
	
	pthread_t interactive_thread;
	pthread_create(&interactive_thread, NULL, interactiveServerThread, NULL);

	if (pthread_mutex_init(&bufferLock, NULL) != 0)
	{
		printf("Mutex init failed\n");
		exit(1);
	}
	getchar();
	system("pause");
	
	return 0;
}