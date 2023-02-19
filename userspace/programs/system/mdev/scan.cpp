#include "scan.hpp"

#include <fcntl.h>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>

#include <sys/wait.h>
#include <unistd.h>

int modprobe(const std::string& module, std::string& resolvedModuleName) {
    int stdoutPipePair[2];
    if(pipe(stdoutPipePair)) {
        std::cerr << "Call to pipe() failed" << std::endl;
        exit(-1);
    }

    int child = fork();
    if(!child) {
        // This is the child process.
        const char* args[] = { "modprobe", "-R", module.c_str(), 0 };

        // Redirect write end of the pipe to stdout
        dup2(stdoutPipePair[1], 1);
        // Close read end of the pipe
        close(stdoutPipePair[0]);
        // Close the other write end of the pipe
        close(stdoutPipePair[1]);

        execvp("modprobe", const_cast<char**>(args));
        exit(-1);
    } else {
        // This is the parent process
        // Close write end of the pipe
        close(stdoutPipePair[1]);

        std::string result;
        char tempBuffer[256];

        int c;
        // This will read until the child process closes stdout (basically until it exits)
        while((c = read(stdoutPipePair[0], tempBuffer, 255))) {
            tempBuffer[c] = 0;
            result += tempBuffer;
        }

        int status;
        // Fetch exit code of the child
        waitpid(child, &status, 0);
        close(stdoutPipePair[0]);

        // All that modprobe should output is the module name.
        resolvedModuleName = result.substr(0, result.length() - 1);

        return status;
    }
}

std::list<std::string> udev::find_busses() {
    std::list<std::string> busList;

    auto busIter = std::filesystem::directory_iterator("/sys/bus");
    for(auto& entry : busIter) {
        if(entry.is_directory()) {
            busList.push_back(entry.path().filename());
        }
    }

    return busList;
}

void udev::probe_bus(std::string bus) {
    auto busIter = std::filesystem::directory_iterator("/sys/bus/" + bus + "/devices");

    for(auto& device : busIter) {
        std::ifstream ueventStream(device.path() / "uevent");

        if(ueventStream.is_open()) {
            std::string line;
            while(std::getline(ueventStream, line)) {
                if(line.find("MODALIAS") == 0) {
                    // Line starts with 'MODALIAS'
                    auto alias = line.substr(9);
                    std::cout << "Loading module for alias '" << alias << "'" << std::endl;

                    std::string resultName;
                    int status = modprobe(alias, resultName);
                    if(!status) {
                        // Succesfully loaded the module
                        std::cout << "Loaded module '" << resultName << "' for device " << device.path().filename() << std::endl;
                        int fd = open((device.path() / "attach").c_str(), O_RDONLY);
                        write(fd, resultName.c_str(), resultName.length());
                        close(fd);
                    }
                    break;
                }
            }
        }
    }
}

