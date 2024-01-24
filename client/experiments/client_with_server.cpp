// Initially I planned on calling the C++ from Python by communicating through
// a socket. That was unnecessary -- it was easier to call the C++ directly
// from Python using PyBind11.
// For history's sake, here's the brightness client modified to wait for
// connections on a socket.
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

// Deps for server
#include <netinet/in.h>
#define PORT 3937


int ParseBrightness(int argc, char **argv, long &brightness) {
    if (argc != 2) {
        return 1;
    }
    char *p;
    brightness = strtol(argv[1], &p, 10);
    if (brightness < 0 || brightness > 900) {
        return 1;
    }
    return 0;
}


int startserver() {
    std::cout << "HERE" << std::endl;
    int server_fd, new_socket;
    ssize_t valread;
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);
    char buffer[1024] = { 0 };
    char* hello = "Hello from server";

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    /*
    if (setsockopt(server_fd, SOL_SOCKET,
                   SO_REUSEADDR | SO_REUSEPORT, &opt,
                   sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }*/
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr*)&address,
             sizeof(address))
        < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    if ((new_socket
         = accept(server_fd, (struct sockaddr*)&address,
                  &addrlen))
        < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }
    do {
        valread = read(new_socket, buffer,
                       1024 - 1); // subtract 1 for the null
                                  // terminator at the end
        printf("%s\n", buffer);
    } while (valread);
    send(new_socket, hello, strlen(hello), 0);
    printf("Hello message sent\n");

    // closing the connected socket
    close(new_socket);
    // closing the listening socket
    close(server_fd);
    return 0;
}


int main(int argc, char **argv) {
    startserver();
    exit(0);
    long brightness;
    struct sockaddr_ctl       addr;

    int error = ParseBrightness(argc, argv, brightness);
    if (error) {
        std::cout << "Usage: ./brightness [value between 0-900]" << std::endl;
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
