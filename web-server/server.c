#include "helper.h"
#include "request.h"
#include <pthread.h>

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
int buffer_size;
int *buffer;
void getargs(int *port, int *num_threads, char *shm_name, int argc, char *argv[])
{
  if (argc != 5) {
    fprintf(stderr, "Usage: %s [port_num] [threads] [buffers] [shm_name]\n", argv[0]);
    exit(1);
  }
  *port = atoi(argv[1]);
  *num_threads = atoi(argv[2]);
  buffer_size = atoi(argv[3]);
  shm_name = argv[4];

  // do we check port?
  if (*num_threads <= 0) {
    fprintf(stderr, "Number of threads is not a positive integer\n");
    exit(1);
  }
  if (buffer_size <= 0) {
    fprintf(stderr, "Buffers is not a positive integer\n");
    exit(1);
  }
}

void shift(int *buffer) {
  for (int i = 1; i < buffer_size; i++) {
    buffer[i-1] = i;
  }
  buffer[buffer_size-1] = -1;
}


void *worker(void *arg) {
  // printf("Worker thread %d starts\n", (uintptr_t)arg);
  while (1) {
    pthread_mutex_lock(&mutex);
    // if buffer is empty, wait til producer puts something in
    while (count == 0) {
      pthread_cond_wait(&empty, &mutex);
    }
    int connfd = buffer[0];
    count--;
    shift(buffer);
    pthread_cond_signal(&full);
    pthread_mutex_unlock(&mutex);
    requestHandle(connfd);
    Close(connfd);

  }
  return 0;
}



void add_to_buffer(int fd) {
  // input will be connfd
  pthread_mutex_lock(&mutex);
  while (count >= buffer_size) {
    pthread_cond_wait(&full, &mutex);
  }

  buffer[tail] = fd;
  count++;
  tail++;

  // not empty anymore
  pthread_cond_signal(&empty);
  pthread_mutex_unlock(&mutex);

}

int main(int argc, char *argv[])
{
  int listenfd, connfd, port, clientlen, num_threads;
  char *shm_name;
  struct sockaddr_in clientaddr;

  getargs(&port, &num_threads, shm_name, argc, argv);

  //
  // CS537 (Part B): Create & initialize the shared memory region...
  //

  // int shm_fd = shm_open(shm_name, O_RDWR | O_CREAT, 0660);
  // void *shm_ptr = mmap(NULL, <pagesize>, PROT_READ | PROT_WRITE,
  // MAP_SHARED, shm_fd, 0);

  // 
  // CS537 (Part A): Create some threads...
  //
  // create a buffer
  // 1 producer thread
  // pool of consumer threads

  buffer = (int *) malloc(sizeof(int) * buffer_size);
  pthread_t thread_pool[num_threads];
  
  count = 0;
  tail = 0;

  pthread_mutex_init(&mutex, NULL);
  pthread_cond_init(&empty, NULL);
  pthread_cond_init(&full, NULL);

  for (int i = 0; i < buffer_size; i++) {
    buffer[i] = -1;
  }
  for (int i = 0; i < num_threads; i++) {
    pthread_create(&thread_pool[i], NULL, worker, NULL);
  }



  listenfd = Open_listenfd(port);
  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
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
