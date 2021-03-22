/*
 * This user app opens the uio device created by the module, map the memory region of this device and wait for interrupts.
 * You can launch the user app with: sudo ./uio_user "/dev/uio0"
 */

/* Includes */
/**
 * ...
 */

#define MMAP_SIZE 0x1000 //size of a pafe -16³ : 4K-

int main(int argc, char *argv[])
{
  int retCode = 0;
  int uioFd; //The file descriptor on the device

  /*
   * A variable should be declared volatile whenever its value could change unexpectedly. In practice, only three types of variables could change:
   * 1. Memory-mapped peripheral registers
   * 2. Global variables modified by an interrupt service routine
   * 3. Global variables accessed by multiple tasks within a multi-threaded application 
   */
  volatile uint64_t *counters;

  // Open uio device
  uioFd = open(/*...*/);
  if (uioFd < 0)
  {
    /*...code...*/
  }

  // Mmap the memory region of the device
  counters = mmap(/*...*/);
  if (counters == MAP_FAILED)
  {
    /*...code...*/
  }

  // Interrupt loop
  while (1)
  {
    uint32_t intInfo;
    ssize_t readSize;

    // Wait for interrupt: the read blocks till an interrupt is received
    readSize = read(/*...*/);
    if (readSize < 0)
    {
      /*...code...*/
    }

    // You can print the contain of the mmaped memory -- intInfo will contain the number of signals till the last interrupt
    /*...*/
  }

  // Should never reach
  munmap((void *)counters, MMAP_SIZE);
  close(uioFd);

  return retCode;
}
