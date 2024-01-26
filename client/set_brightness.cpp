// Simple C++ client that connects to the kernel extension, sets the brightness
// level, and disconnects.
//
// Compile with
//    make set_brightness.out
//
// Usage
//    ./build/set_brightness [100-900]
//
// Brightness 0 is not allowed because then you'd be stuck with a black screen.

#include <iostream>
#include <stdlib.h>  // For strtol.

#include "kernel_socket.hpp"


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

    int error = ParseBrightness(argc, argv, brightness);
    if (error) {
        std::cout << "Usage: ./brightness [value between 100-900]" << std::endl;
        exit(0);
    }
    KernelSocket kernel_socket;
    if (!kernel_socket.connect()) {
        std::cout << "Connection to kernel extension failed." << std::endl;
        // Exit right away, as the connection isn't open.
        exit(0);
    }
    if (!kernel_socket.setBrightness(brightness)) {
        std::cout << "Failed to set brightness." << std::endl;
        // Don't exit -- fall through and let it close the connection.
    }
    kernel_socket.close();
    return 0;
}
