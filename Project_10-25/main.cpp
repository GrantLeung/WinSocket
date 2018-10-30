#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <Windows.h>
extern "C" {
#include "ladder.h"
}

pthread_mutex_t bufferLock;
pthread_mutex_t logLock; //mutex for the internal log
char log_buffer[1000000]; //A very large buffer to store all logs
int log_index = 0;
int log_counter = 0;
//void startInteractiveServer(int);

void log(unsigned char *logmsg)
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

void* interactiveServerThread(void*)
{
	startInteractiveServer(43628);
	return NULL;
}

int main()
{
	unsigned char log_msg[1000];
	sprintf((char*)log_msg, "OpenPLC Runtime starting...\n");

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