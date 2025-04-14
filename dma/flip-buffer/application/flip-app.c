#include <stdio.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#define BUFFER_SIZE 4096
#define DMAFLIP_IOCTL_GET_READY _IOR('D', 1, int)

int main()
{
    int fd = open("/dev/dmaflip", O_RDONLY);
    if (fd < 0)
    {
        perror("open");
        return 1;
    }

    char *mapped = mmap(NULL, BUFFER_SIZE * 2, PROT_READ, MAP_SHARED, fd, 0);
    if( mapped == MAP_FAILED )
    {
        perror("mmap");
        return 1;
    }

    for( int i = 0; i < 10; i++ )
    {
        int ready;
        if( ioctl( fd, DMAFLIP_IOCTL_GET_READY, &ready) < 0 )
        {
            perror("ioctl");
            break;
        }

        int read_idx = ready ^ 1;  // Lese aus dem nicht mehr aktiven Puffer
        printf("User liest Buffer %d:\n%s", read_idx, mapped + read_idx * BUFFER_SIZE);
        sleep(1);
    }

    munmap(mapped, BUFFER_SIZE * 2);
    close(fd);
    return 0;
}

