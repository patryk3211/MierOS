#include <cstdlib>
#include <iostream>
#include <unistd.h>

int main(int argc, char* argv[]) {
    // Since we are the first process executed, the environment
    // will be empty and we need to populate it.
    setenv("PATH", "/sbin", true);

    int child = fork();
    printf("child = %d", child);
    if(!child) {
        char* args[] = { "modprobe", "pc_serial", 0 };
        execvp("modprobe", args);
    }
    //system("modprobe pc_serial");

    while(true);
    return 0;
}
