// Compile with
// g++ -o brightness client_simple.cpp
// Usage
// ./brightness 700
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

#include "kernel_socket.hpp"

// Special commands to increase or decrease current brightness by a fixed
// amount.
#define BRIGHTNESS_UP   -100
#define BRIGHTNESS_DOWN -200

#define MIN_BRIGHTNESS  100
// Although the Linux kernel uses a much higher value (0xffff) as the max,
// and the Mux doesn't seem to have a problem setting it, it is useless.
// The actual brightness does not increase further if set above 900.
#define MAX_BRIGHTNESS  900


bool KernelSocket::connect() {
    struct sockaddr_ctl addr;
    fd = socket(PF_SYSTEM, SOCK_DGRAM, SYSPROTO_CONTROL);
    if (fd == -1) { /* no fd */
        fprintf(stderr, "failed to open socket\n");
        return false;
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
            ::close(fd);
            return false;
        }
        else {
            std::cout << "ID for kernel control: " << info.ctl_id << std::endl;
            // std::cout << "QUITTING BEFORE GOING FURTHER" << std::endl;
            // ::close(fd);
            // return true;
        }
        addr.sc_id = info.ctl_id;
        addr.sc_unit = 0;
    }

    int error = ::connect(fd, (struct sockaddr *)&addr, sizeof(addr));
    if (error) {
       fprintf(stderr, "connect failed %d\n", error);
       ::close(fd);
       return false;
    }
    return true;
}

bool KernelSocket::setBrightness(int value) {
    if (value != BRIGHTNESS_UP && value != BRIGHTNESS_DOWN
        && (value < MIN_BRIGHTNESS || value > MAX_BRIGHTNESS)) {
        std::cout << "Invalid brightness value: " << value << std::endl;
        return false;
    }
    // Only print this log if setting an actual value. Don't log if value is
    // one of the special commands -100 and -200 for brightness up/down, 
    if (value >= MIN_BRIGHTNESS) {
        std::cout << "Setting brightness to: " << value << std::endl;
    }
    int error = setsockopt(fd, SYSPROTO_CONTROL, value, NULL, 0);
    if (error){
        fprintf(stderr, 
            "setsockopt failed to send brightness - error was %d\n", error);
    }
    return true;
}

bool KernelSocket::increaseBrightness() {
    return setBrightness(BRIGHTNESS_UP);
}

bool KernelSocket::decreaseBrightness() {
    return setBrightness(BRIGHTNESS_DOWN);
}

void KernelSocket::close() {
    std::cout << "Closing connection" << std::endl;
    ::close(fd);
}

