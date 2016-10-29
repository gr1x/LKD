#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <signal.h>

#include "common.h"



int main(){
   int ret, fd;
   char buf[128];

   fd = open("/dev/sigchar", O_RDWR); 
   if (fd < 0){
      perror("Failed to open the device...");
      return errno;
   }
   system("dmesg -c");


   printf("Send singal to kthread...\n");
   ret = write(fd, buf, 128); 
   if (ret < 0){
      perror("Failed to write the message to the device.");
      return errno;
   }
   system("dmesg -c");
   getchar();
   printf("stop kthread ...\n");
   ret = read(fd, buf, 128); 
   if (ret < 0){
      perror("Failed to read the message from the device.");
      return errno;
   }
   system("dmesg -c");


   printf("End of the program\n");
   return 0;
}