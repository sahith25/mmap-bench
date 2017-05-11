#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <chrono>
#include <vector>
#include <random>
#include <sys/mman.h>
#include <errno.h>
#include <algorithm>
using namespace std;

long long B_SIZE = 4096;
long long MIN_FILE_SIZE = (32 * 1024) * B_SIZE;
long long FILE_SIZE;
bool MMAP = false;
int ADVICE = MADV_NORMAL;

int preprocess(int argc, char *argv[])
{
	if (argc < 4)
	{
		printf("usage : %s <file> <mode> <method> [advice]\n", argv[0]);
		exit(0);
	}

	if(strcmp(argv[2], "seq") != 0 && strcmp(argv[2], "random") != 0)
	{
		printf("<mode> must be `random` or `seq`\n");
		exit(0);
	}

	if(strcmp(argv[3], "read") != 0 && strcmp(argv[3], "mmap") != 0)
	{
		printf("<method> must be `read` or `mmap`\n");
		exit(0);
	}

	if (strcmp(argv[3], "mmap") == 0)
		MMAP = true;

	if (argc >= 5)
	{	
		if (MMAP)
		{
			if(strcmp(argv[4], "adv_seq") != 0 && strcmp(argv[4], "adv_random") != 0)
			{
				printf("[advice] must be `adv_seq` or `adv_random`\n");
				exit(0);
			}

			if (strcmp(argv[4], "adv_seq") == 0)
				ADVICE = MADV_SEQUENTIAL;
			else if (strcmp(argv[4], "adv_random") == 0)
				ADVICE = MADV_RANDOM;
			else
				ADVICE = MADV_NORMAL;
		}
	}

	int fd = open(argv[1], O_RDONLY);
	if (fd < 0)
	{
		printf("could not open file %s\n", argv[1]);
		exit(0);
	}

	FILE_SIZE = lseek(fd, 0, SEEK_END);
	if (FILE_SIZE < MIN_FILE_SIZE)
	{
		printf("file too small with only %lld bytes\nfile should be atleast %lld bytes\n", FILE_SIZE, MIN_FILE_SIZE);
		exit(0);
	}
	printf("opened file %s with %lld bytes\n", argv[1], FILE_SIZE);
	lseek(fd, 0, SEEK_SET);

	return fd;
}


int main(int argc, char *argv[])
{
	int fd = preprocess(argc, argv);	

	char buf[B_SIZE];
	long long NUM_B = FILE_SIZE / B_SIZE;

	vector<int> num(NUM_B);
	for (int i = 0; i < NUM_B; i++)
		num[i] = i;

	if (strcmp(argv[2], "random") == 0)
	{
		srand(unsigned(time(NULL)));
		random_shuffle(num.begin(), num.end());
	}

	char* data, bogus = 0;
	
	if (MMAP)
	{
		data = (char*)mmap(NULL, NUM_B*B_SIZE, PROT_READ, MAP_SHARED, fd, 0);
		if ((long)data < 0)
		{
			printf("mmap error : %s\n", strerror(errno));
			return 0;
		}
		int mm = madvise(data, NUM_B*B_SIZE, ADVICE | MADV_WILLNEED);
	}
	
	auto t1 = chrono::high_resolution_clock::now();
	if (MMAP)
	{
		for (int b = 0; b < NUM_B; b++)
		{	
			for (int aa = 0; aa < 20; aa++)
				for (int i = 0; i < B_SIZE; i++)
					bogus += data[num[b]*B_SIZE + i];
		}
	}
	else
	{
		for (int b = 0; b < NUM_B; b++)
		{	
			lseek(fd, num[b]*B_SIZE, SEEK_SET);
			int nr = read(fd, buf, B_SIZE);

			for (int aa = 0; aa < 20; aa++)
				for (int i = 0; i < B_SIZE; i++)
					bogus += buf[i];
		}
	}

    auto t2 = chrono::high_resolution_clock::now();
    auto dur = chrono::duration_cast<chrono::milliseconds>(t2 - t1).count();

    printf("took %ldms\n", dur);
    printf("iops : %.2f\n", (double)(NUM_B)/dur);
    printf("bogus %d\n", bogus);

    if (MMAP)
    {
    	munmap(data, NUM_B*B_SIZE);
    }

	return 0;
}