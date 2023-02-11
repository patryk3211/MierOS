#include <iostream>
#include <unistd.h>

int main(int argc, char* argv[]) {
    if(getpid() != 1) {
        std::cerr << "This process can only be started by the kernel!\n";
        return -1;
    }

    std::cout << "Welcome to MInit! A simple MierOS system init daemon." << std::endl;

    // Initialize some environment variables
    setenv("PATH", "/bin:/usr/bin:/sbin", true);
    setenv("USER", "root", true);


    while(true);
}

