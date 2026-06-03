#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <time.h>

#define BLOCK_SIZE 4096       
#define NUM_REQUESTS 100      
#define DISK_SIZE_MB 1024     

void sequential_access_worker(const char *device_path, int worker_id) {
    int fd = open(device_path, O_RDONLY);
    if (fd < 0) {
        perror("Error opening disk");
        exit(1);
    }
    
    char buffer[BLOCK_SIZE];
    
    off_t offset = worker_id * NUM_REQUESTS * BLOCK_SIZE; 
    
    for (int i = 0; i < NUM_REQUESTS; i++) {
        pread(fd, buffer, BLOCK_SIZE, offset);
        offset += BLOCK_SIZE;
    }
    
    close(fd);
}

void random_access_worker(const char *device_path, int worker_id) {
    int fd = open(device_path, O_RDONLY);
    if (fd < 0) {
        perror("Error opening disk");
        exit(1);
    }
    
    char buffer[BLOCK_SIZE];
    
    srand(time(NULL) ^ (getpid() << 16)); 
    
    long long max_blocks = (DISK_SIZE_MB * 1024LL * 1024LL) / BLOCK_SIZE;
    
    for (int i = 0; i < NUM_REQUESTS; i++) {
        off_t offset = (rand() % max_blocks) * BLOCK_SIZE;
        pread(fd, buffer, BLOCK_SIZE, offset);
    }
    
    close(fd);
}

int main(int argc, char** argv) {
    if(argc < 3) {
        printf("Usage: %s <disk_path> <test_type> [num_processes]\n", argv[0]);
        printf("Test Type: 1 - Sequential Access | 2 - Random Access\n");
        printf("Example: %s /dev/sdb 2 5\n", argv[0]);
        return -1; 
    }

    const char      *device_path = argv[1];
    int             test_type    = atoi(argv[2]);
    int             worker_count = 5; 
    struct timespec ts;
    uint64_t        start_time = 0;

    if (argc == 4) {
        worker_count = atoi(argv[3]);
        if (worker_count <= 0) worker_count = 1;
    }

    if(clock_gettime(CLOCK_REALTIME, &ts) == 0){
        start_time = (uint64_t)ts.tv_sec * 1000000000LL + ts.tv_nsec; 
    }

    printf("Starting %s test on disk '%s' with %d processes...\n", 
           test_type == 1 ? "Sequential" : "Random", device_path, worker_count);

    for (int i = 0; i < worker_count; i++) {
        pid_t pid = fork();
        
        if (pid < 0) {
            perror("Fork failed");
            exit(1);
        }  
        if (pid == 0) {
            if (test_type == 1) {
                sequential_access_worker(device_path, i);
            }  
            if (test_type == 2) {
                random_access_worker(device_path, i);
            }
            
            exit(0);
        }
    }

    for (int i = 0; i < worker_count; i++) {
        wait(NULL);
    }

    uint64_t stop_time = 0;

    if(clock_gettime(CLOCK_REALTIME, &ts) == 0){
        stop_time = (uint64_t)ts.tv_sec * 1000000000LL + ts.tv_nsec; 
    }

    uint64_t dt = stop_time - start_time;

    printf("Test finished! Time Elapsed: %llu\n", dt);
    return 0;
}