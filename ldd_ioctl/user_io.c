#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <signal.h>

#include "common.h"


#define BUFFER_LENGTH 256 
static char receive[BUFFER_LENGTH];
struct ioctl_data_t io_data = {0};

void read_device(int n, siginfo_t *info, void *unused)
{
   printf("Signal coming, read device %i\n", info->si_int);
}


int main(){
   int ret, fd;
   char stringToSend[BUFFER_LENGTH];
   printf("%d \n", SIGPROF);
   fd = open("/dev/gchar", O_RDWR); 
   if (fd < 0){
      perror("Failed to open the device...");
      return errno;
   }
   printf("Type in a short string to send to the kernel module:\n");
   scanf("%[^\n]%*c", stringToSend); 
   printf("Writing message to the device [%s].\n", stringToSend);
   ret = write(fd, stringToSend, strlen(stringToSend)); 
   if (ret < 0){
      perror("Failed to write the message to the device.");
      return errno;
   }
   
   printf("Press ENTER to read back from the device...\n");
   getchar();
   
   printf("Reading from the device...\n");
   ret = read(fd, receive, BUFFER_LENGTH); 
   if (ret < 0){
      perror("Failed to read the message from the device.");
      return errno;
   }
   printf("The received message is: [%s]\n", receive);

   printf("Reset device\n");
   ret = ioctl(fd, GCHAR_IOC_RESET);

   pid_t pid = getpid();
   memcpy(&io_data.pid, &pid, sizeof(pid_t));
   ret = ioctl(fd, GCHAR_IOC_WRITE, &io_data);
   printf("Write RET: %d\n", ret);

   struct sigaction sig;
   sig.sa_sigaction = read_device;
   sig.sa_flags = SA_SIGINFO;
   sigaction(SIG_READ, &sig, NULL);

   ret = ioctl(fd, GCHAR_IOC_READ, &io_data);
   printf("Read RET: %d\n", ret);

   printf("End of the program\n");
   return 0;
}