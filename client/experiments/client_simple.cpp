// Simple client that connects to the kernel extension, sets the brightness
// level, and disconnects.
//
// Compile with
//    g++ -o client_simple.out client_simple.cpp
//
// Usage
//    ./client_simple.out 700
// zero is not allowed because then you're stuck with a black screen.

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/sys_domain.h>
#include <sys/kern_control.h>
#include <net/if_utun.h>
#include <iostream>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>  // for strtol
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include <stdlib.h> // exit, etc.


int ParseBrightness(int argc, char **argv, long &brightness) {
    if (argc != 2) {
        return 1;
    }
    char *p;
    brightness = strtol(argv[1], &p, 10);
    if (brightness < 100 || brightness > 900) {
        return 1;
    }
    return 0;
}


int main(int argc, char **argv) {
    long brightness;
    struct sockaddr_ctl       addr;

    int error = ParseBrightness(argc, argv, brightness);
    if (error) {
        std::cout << "Usage: ./brightness [value between 100-900]" << std::endl;
        exit(0);
    }
 
    int fd = socket(PF_SYSTEM, SOCK_DGRAM, SYSPROTO_CONTROL);
    if (fd == -1) { /* no fd */
            fprintf(stderr, "failed to open socket\n");
            exit(-1);
    }
    bzero(&addr, sizeof(addr)); // sets the sc_unit field to 0
    addr.sc_len = sizeof(addr);
    addr.sc_family = AF_SYSTEM;
    addr.ss_sysaddr = AF_SYS_CONTROL;
    {
        struct ctl_info info;
        memset(&info, 0, sizeof(info));
        strncpy(info.ctl_name, "org.mklinux.nke.NVDAGPUWakeHandler", sizeof(info.ctl_name));
        if (ioctl(fd, CTLIOCGINFO, &info)) {
            perror("Could not get ID for kernel control.\n");
            close(fd);
            exit(-1);
        }
        else {
            std::cout << "ID for kernel control: " << info.ctl_id << std::endl;
            // std::cout << "QUITTING BEFORE GOING FURTHER" << std::endl;
            // close(fd);
            // exit(0);
        }
        addr.sc_id = info.ctl_id;
        addr.sc_unit = 0;
    }

    error = connect(fd, (struct sockaddr *)&addr, sizeof(addr));
    if (error) {
       fprintf(stderr, "connect failed %d\n", error);
       close(fd);
       exit(-1);
    }
 
    std::cout << "Setting brightness to: " << brightness << std::endl;
    error = setsockopt(fd, SYSPROTO_CONTROL, brightness, NULL, 0);
    if (error){
        fprintf(stderr, 
            "setsockopt failed to send brightness - error was %d\n", error);
    }
    close(fd);
    return (0);
}
