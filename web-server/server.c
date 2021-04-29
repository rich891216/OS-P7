#include "helper.h"
#include "request.h"
#include <pthread.h>
#include "shm_slot.h"

//
// server.c: A very, very simple web server
//
// To run:
//  server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//

// CS537: Parse the new arguments too

pthread_cond_t empty, full;
pthread_mutex_t mutex;
int count;
int tail;
slot_t *shm_slot_ptr;

int *buffer;
char *shm_name;
int buffer_size;
int num_threads;
int pagesize;

void getargs(int *port, int *num_threads, int *buffer, char **shm_name, int argc, char *argv[])
{
	if (argc != 5)
	{
		fprintf(stderr, "Usage: %s [port_num] [threads] [buffers] [shm_name]\n", argv[0]);
		exit(1);
	}
	*port = atoi(argv[1]);
	*num_threads = atoi(argv[2]);
	*buffer = atoi(argv[3]);
	*shm_name = (char *) argv[4];

	// do we check port?
	if (*num_threads <= 0)
	{
		fprintf(stderr, "Number of threads is not a positive integer\n");
		exit(1);
	}
	if (buffer_size <= 0)
	{
		fprintf(stderr, "Buffers is not a positive integer\n");
		exit(1);
	}
}

void shift(int *buffer)
{
	for (int i = 1; i < buffer_size; i++)
	{
		buffer[i - 1] = i;
	}
	buffer[buffer_size - 1] = -1;
}

void *worker(void *arg)
{
	// printf("Worker thread %d starts\n", (uintptr_t)arg);
	while (1)
	{
		pthread_mutex_lock(&mutex);
		// if buffer is empty, wait til producer puts something in
		while (count == 0)
		{
			pthread_cond_wait(&empty, &mutex);
		}
		int connfd = buffer[0];
		count--;
		shift(buffer);
		pthread_cond_signal(&full);
		pthread_mutex_unlock(&mutex);
		int request_type = requestHandle(connfd);
		int index = -1;

		for (int i = 0; i < num_threads; i++) {
			if (shm_slot_ptr[i].id == pthread_self()) {
				index = i;
				break;
			}
		}

		if (index == -1) {
			index = num_threads;
			shm_slot_ptr[index].id = pthread_self();
		}
		
		shm_slot_ptr[index].requests++;

		if (request_type == 0) {
			shm_slot_ptr[index].s_req++;
		}

		if (request_type == 1) {
			shm_slot_ptr[index].d_req++;
		}
		
		Close(connfd);
	}
	return 0;
}

void add_to_buffer(int fd)
{
	// input will be connfd
	pthread_mutex_lock(&mutex);
	while (count >= buffer_size)
	{
		pthread_cond_wait(&full, &mutex);
	}

	buffer[tail] = fd;
	count++;
	tail++;

	// not empty anymore
	pthread_cond_signal(&empty);
	pthread_mutex_unlock(&mutex);
}

void int_handler () {
	int ret1 = munmap(NULL, pagesize);
	if (ret1 != 0) {
		perror("munmap failed.\n");
		exit(1);
	}

	int ret2 = shm_unlink(shm_name);
	if (ret2 != 0) {
		perror("shm_unlink failed.\n");
		exit(1);
	}
	exit(0);
}

int main(int argc, char *argv[])
{
	int listenfd, connfd, port, threads, buffers, clientlen;

	struct sockaddr_in clientaddr;
	pagesize = getpagesize();

	getargs(&port, &threads, &buffers, &shm_name, argc, argv);

	//
	// CS537 (Part B): Create & initialize the shared memory region...
	//

	int shm_fd = shm_open(shm_name, O_RDWR | O_CREAT, 0660);
	if (shm_fd < 0)
	{
		perror("shm_open failed.\n");
		exit(1);
	}

	int ret = ftruncate(shm_fd, pagesize);
    
	if (ret != 0) {
    	exit(1);
  	}

	shm_slot_ptr = mmap(NULL, pagesize, PROT_READ | PROT_WRITE,
				   MAP_SHARED, shm_fd, 0);
	
	if (shm_slot_ptr == MAP_FAILED) {
		perror("mmap failure.\n");
		exit(1);
	}

	signal(SIGINT, int_handler);

	//
	// CS537 (Part A): Create some threads...
	//
	// create a buffer
	// 1 producer thread
	// pool of consumer threads

	buffer_size = buffers;
	int work_buf[buffer_size];
	buffer = work_buf;
	pthread_t thread_pool[threads];
	num_threads = threads;

	count = 0;
	tail = 0;

	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&empty, NULL);
	pthread_cond_init(&full, NULL);

	for (int i = 0; i < threads; i++)
	{
		pthread_create(&thread_pool[i], NULL, worker, &work_buf);
	}

	listenfd = Open_listenfd(port);
	while (1)
	{
		clientlen = sizeof(clientaddr);
		connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *)&clientlen);
		add_to_buffer(connfd);

		//
		// CS537 (Part A): In general, don't handle the request in the main thread.
		// Save the relevant info in a buffer and have one of the worker threads
		// do the work. Also let the worker thread close the connection.
		//
		// requestHandle(connfd);
		// Close(connfd);
	}
}
