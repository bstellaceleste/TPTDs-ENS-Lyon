/*
 * This user app opens the uio device created by the module, map the memory region of this device and wait for interrupts.
 * You can launch the user app with: sudo ./uio_user "/dev/uio0"
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>

#define MMAP_SIZE     0x1000 //size of a pafe 16³

int main(int argc, char* argv[])
{
  int retCode = 0;
  int uioFd;
  char *UIO_DEVICE = argv[1];
  /*
   * A variable should be declared volatile whenever its value could change unexpectedly. In practice, only three types of variables could change:
   * 1. Memory-mapped peripheral registers
   * 2. Global variables modified by an interrupt service routine
   * 3. Global variables accessed by multiple tasks within a multi-threaded application 
   */
  volatile uint64_t* counters;

  // Open uio device
  uioFd = open(UIO_DEVICE, O_RDWR);
  if(uioFd < 0)
  {
    fprintf(stderr, "Cannot open %s: %s\n", UIO_DEVICE, strerror(errno));
    return -1;
  }

  // Mmap memory region containing values 
  counters = mmap(NULL, MMAP_SIZE, PROT_READ, MAP_SHARED, uioFd, 0);
  if(counters == MAP_FAILED)
  {
    fprintf(stderr, "Cannot mmap: %s\n", strerror(errno));
    close(uioFd);
    return -1;
  }
  printf("%lx\n", counters);
  // Interrupt loop
  while(1)
  {
    uint32_t intInfo;
    ssize_t readSize;

    // Acknowldege interrupt
    intInfo = 1;

    // Wait for interrupt: the read blocks till an interrupt is received
    readSize = read(uioFd, &intInfo, sizeof(intInfo));
    printf("waiting for interrupt- readsize = %d\n", readSize);
    if(readSize < 0)
    {
      fprintf(stderr, "Cannot wait for uio device interrupt: %s\n",
        strerror(errno));
      retCode = -1;
      break;
    }

    // Display counter value -- intInfo will contain the number of signals till the last interrupt
    printf("We got %lu interrupts, counter value: 0x%08lx\n",
      intInfo, counters[0]);
  }

  // Should never reach
  munmap((void*)counters, MMAP_SIZE);
  close(uioFd);

  return retCode;
}

