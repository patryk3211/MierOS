#include <iostream>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/mount.h>
#include <fcntl.h>

int cmd(const char* cmd, const char** args) {
    int child = fork();
    if(!child) {
        size_t argc = 0;
        if(args != 0)
            while(args[argc]) ++argc;

        char* argCopy[argc + 2];
        argCopy[0] = const_cast<char*>(cmd);
        for(size_t i = 0; i < argc; ++i)
            argCopy[i + 1] = const_cast<char*>(args[i]);
        argCopy[argc + 1] = 0;

        execvp(cmd, argCopy);
        exit(-1);
    } else {
        waitpid(child, 0, 0);
    }

    return 0;
}

int main(int argc, char* argv[]) {
    if(getpid() != 1) {
        std::cerr << "This process can only be started by the kernel!" << std::endl;
        return -1;
    }

    std::cout << "Welcome to MInit! A simple MierOS system init daemon." << std::endl;

    // Initialize some environment variables
    setenv("PATH", "/bin:/usr/bin:/sbin", true);
    setenv("USER", "root", true);

    // Mount kernel filesystems (/dev, /sys) on initrd
    if(mount(0, "/dev", "devfs", 0, 0))
        std::cerr << "Failed to mount /dev" << std::endl;
    if(mount(0, "/sys", "sysfs", 0, 0))
        std::cerr << "Failed to mount /sys" << std::endl;

    const char* args[] = { "-a", "pci", "pc_serial", 0 };
    cmd("modprobe", args);

    /// Test start mdev
    //cmd("mdev", 0);

    //cmd("dash", 0);
    int child = fork();
    if(!child) {
        int in = open("/dev/ttyS0", O_RDONLY);
        if(in == -1) {
            fprintf(stderr, "Failed to open /dev/ttyS0\n");
            exit(-1);
        }
        int out = open("/dev/ttyS0", O_WRONLY);

        dup2(in, 0);
        dup2(out, 1);
        dup2(out, 2);
        close(in);
        close(out);

        char* arg[] = { const_cast<char*>("dash"), 0 };
        execvp("dash", arg);
        exit(-1);
    } else {
        waitpid(child, 0, 0);
    }

    while(true)
        asm volatile("rep nop");
}

