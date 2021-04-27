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
void getargs(int *port, int argc, char *argv[])
{
  if (argc != 5) {
    fprintf(stderr, "Usage: %s [port_num] [threads] [buffers] [shm_name]\n", argv[0]);
    exit(1);
  }
  *port = atoi(argv[1]);
}

pthread_t producer;
pthread_cond_t empty, full;
pthread_mutex_t mutex;

void *worker(void *arg) {
  printf("Worker thread %d starts\n", (uintptr_t)arg);

  return 0;
}

int main(int argc, char *argv[])
{
  int listenfd, connfd, port, clientlen;
  struct sockaddr_in clientaddr;

  getargs(&port, argc, argv);
  int threads = atoi(argv[2]);
  int buffers = atoi(argv[3]);

  char *shm_name = argv[4];

  

  //
  // CS537 (Part B): Create & initialize the shared memory region...
  //
  int shm_fd = shm_open(shm_name, O_RDWR | O_CREAT, 0660);
  void *shm_ptr = mmap(NULL, <pagesize>, PROT_READ | PROT_WRITE,
  MAP_SHARED, shm_fd, 0);

  // 
  // CS537 (Part A): Create some threads...
  //
  // create a buffer
  // 1 producer thread
  // pool of consumer threads


  int buffer[buffers];
  pthread_t thread_pool[threads];


  pthread_mutex_init(&mutex, NULL);
  pthread_cond_init(&empty, NULL);
  pthread_cond_init(&full, NULL);

  for (int i = 0; i < buffers; i++) {
    buffer[i] = 0;
  }
  for (int i = 0; i < threads; i++) {
    pthread_create(&thread_pool[i], NULL, worker, (void *)i)
  }



  listenfd = Open_listenfd(port);
  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);

    // 
    // CS537 (Part A): In general, don't handle the request in the main thread.
    // Save the relevant info in a buffer and have one of the worker threads 
    // do the work. Also let the worker thread close the connection.
    // 
    requestHandle(connfd);
    Close(connfd);
  }
}
