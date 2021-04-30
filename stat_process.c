#include "helper.h"
#include "request.h"
#include <time.h>
#include "shm_slot.h"

int pagesize;
int shm_fd;
char *shm_name;
slot_t *shm_slot_ptr;

// Grab the arguments for the process
void getargs(char **shm_name, long *sleeptime_ms, int *num_threads, int argc, char *argv[])
{
	if (argc != 4)
	{
		fprintf(stderr, "Usage: stat_process [shm_name] [sleeptime_ms] [num_threads]");
		exit(1);
	}
	*shm_name = (char *)argv[1];
	*sleeptime_ms = atoi(argv[2]);
	*num_threads = atoi(argv[3]);
}

int main(int argc, char *argv[])
{
	long sleeptime_ms;
	int num_threads;
	pagesize = getpagesize();

	getargs(&shm_name, &sleeptime_ms, &num_threads, argc, argv);

	// ensure proper inputs
	if (sleeptime_ms < 0 || num_threads < 0)
	{
		exit(1);
	}

	shm_fd = shm_open(shm_name, O_RDWR, 0660);
	if (shm_fd < 0)
	{
		perror("shm_open failed.\n");
		exit(1);
	}

	slot_t *shm_slot_ptr = mmap(NULL, pagesize, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

	if (shm_slot_ptr == MAP_FAILED)
	{
		perror("mmap failure.\n");
		exit(1);
	}

	int count = 1;
	// cycle of print and sleep
	while (1)
	{
		// usleep sleeps for microseconds, so we multiply to get milliseconds
		usleep(sleeptime_ms * 1000);

		for (int i = 0; i < num_threads; i++)
		{
			printf("%d\n%lu : %d %d %d\n", count, shm_slot_ptr[i].id, shm_slot_ptr[i].requests,
				   shm_slot_ptr[i].s_req, shm_slot_ptr[i].d_req);
		}
		count++;
	}
	return 0;
}