#include <cstdio>
#include <iostream>
#include <libopt.h>
#include <list>
#include <sstream>
#include <string>
#include <unordered_map>
#include <fstream>
#include <sys/syscall.h>

struct Module {
    std::string f_moduleName;
    std::string f_moduleFile;
    std::list<std::string> f_dependencies;
};

static std::string module_directory;
static std::unordered_map<std::string, Module> module_map;

static std::unordered_map<std::string, std::string> hashed_aliases;
static std::list<std::pair<std::string, std::string>> pattern_aliases;

void load_module_deps() {
    std::ifstream file(module_directory + "/modules.dep");

    std::string line;
    while(std::getline(file, line)) {
        // Skip comment lines
        if(line[0] == '#')
            continue;

        size_t separator = line.find(':');
        // Skip invalid lines
        if(separator == std::string::npos)
            continue;

        std::string modFile = module_directory + "/" + line.substr(0, separator);
        std::string deps = line.substr(separator + 1);

        auto getModName = [](std::string modFile) -> std::string {
            // Extract module name from the module path
            size_t lastSeparator = modFile.rfind('/');
            size_t lastDot = modFile.rfind('.');
            return modFile.substr(lastSeparator + 1, lastDot - lastSeparator - 1);
        };
        
        std::list<std::string> dependencies;
        std::string dep;
        std::stringstream strstream(deps);
        while(std::getline(strstream, dep, ' ')) {
            if(dep.empty()) continue;

            std::string depName = getModName(dep);
            dependencies.push_back(depName);
        }

        std::string modName = getModName(modFile);
        module_map.insert({ modName, { modName, modFile, std::move(dependencies) } });
    }
}

int strmatch(const char* wildcard, const char* str) {
    const char* text_backup = 0;
    const char* wild_backup = 0;
    while(*str != '\0') {
        if(*wildcard == '*') {
            text_backup = str;
            wild_backup = ++wildcard;
        } else if(*wildcard == '?' || *wildcard == *str) {
            str++;
            wildcard++;
        } else {
            if(wild_backup == 0) return 0;
            str = ++text_backup;
            wildcard = wild_backup;
        }
    }
    while(*wildcard == '*') wildcard++;
    return *wildcard == '\0' ? 1 : 0;
}

void load_module_aliases() {
    std::ifstream file(module_directory + "/modules.alias");

    std::string line;
    while(std::getline(file, line)) {
        // Skip comment files
        if(line[0] == '#')
            continue;

        size_t separator = line.find(' ');
        if(separator == std::string::npos)
            continue;

        std::string alias = line.substr(0, separator);
        std::string modName = line.substr(separator + 1);

        if(alias.find('*') != std::string::npos || alias.find('?') != std::string::npos) {
            pattern_aliases.push_back({ alias, modName });
        } else {
            hashed_aliases.insert({ alias, modName });
        }
    }
}

Module* probe(const std::string& name) {
    // First we check the module name to file location map
    auto hit = module_map.find(name);
    if(hit != module_map.end()) {
        // Found the module in direct mappings
        return &hit->second;
    }

    // If not found, we check the hashed aliases
    auto hit2 = hashed_aliases.find(name);
    if(hit2 != hashed_aliases.end()) {
        // Found the module in hashed aliases,
        // resolve it into the Module struct.
        return probe(hit2->second);
    }

    // Now we check the pattern aliases
    for(auto entry : pattern_aliases) {
        if(strmatch(entry.first.c_str(), name.c_str())) {
            // Matched successfully,
            // resolve further into Module struct.
            return probe(entry.second);
        }
    }

    return 0;
}

void handle_dependencies(Module* module, std::list<Module*>& orderedModules, std::list<Module*>::iterator insertLocation) {
    for(auto& dep : module->f_dependencies) {
        Module* modDep = probe(dep);
        if(!modDep) {
            fprintf(stderr, "Module dependency '%s' of '%s' not found in '%s' directory\n",
                    dep.c_str(), module->f_moduleName.c_str(), module_directory.c_str());
            exit(1);
        }

        // Check if module is already in list,
        // if it is before this position then we can skip adding it
        // but if it is after this position, we need to move it to before this position.
        
        auto foundIter = orderedModules.end();
        bool before = true;
        for(auto iter = orderedModules.begin(); iter != orderedModules.end(); ++iter) {
            if(iter == insertLocation) {
                before = false;
                continue;
            }

            if(modDep == *iter) {
                foundIter = iter;
                break;
            }
        }

        if(foundIter == orderedModules.end()) {
            // Not found, insert and handle it's dependencies
            auto newLocation = orderedModules.insert(insertLocation, modDep);
            handle_dependencies(modDep, orderedModules, newLocation);
        } else if(!before) {
            // Dependency of this module is already loaded after it is,
            // we need to move it down the initialization chain.
            orderedModules.erase(foundIter);
            auto newLocation = orderedModules.insert(insertLocation, modDep);
            // Handle the dependencies to move them down as well
            handle_dependencies(modDep, orderedModules, newLocation);
        }
    }
}

int insmod(const std::string& modFile, const std::string& modName) {
    // We need to read the module file and call init_module
    // Get the file size
    std::ifstream file(modFile, std::ios::ate | std::ios::binary);
    if(!file.is_open()) return -1;

    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    char* buffer = new char[fileSize];
    file.read(buffer, fileSize);

    return syscall(SYS_init_module, buffer, 0);
}

void print_usage(char* progName) {
    fprintf(stderr,
        "Usage:\n" 
        "  %s [options] modulename [moduleargs...]\n"
        "  %s [options] -a modulename [modulename...]\n"
        "\n"
        "Options:\n"
        "  -h, --help - Displays this help message\n"
        "  -v, --verbose - Verbose output\n"
        "  -a, --all - Consider all non-arguments to be a module name\n"
        "  -m, --major - Print major number this module was loaded at\n"
    , progName, progName);
}

static const option options[] = {
    { 'h', "help", 0 },
    { 'v', "verbose", 0 },
    { 'a', "all", 0 },
    { 'm', "major", 0 },
    { 0, 0, 0 }
};


int main(int argc, char* argv[]) {
    auto* apc = start_arg_parse(options, argc, argv);

    std::list<std::string> modList;
    std::list<std::string> argumentList;
    module_directory = "/lib/modules";
    bool verbose = false;
    bool allModules = false;
    bool printMajor = false;
    
    char opt;
    while((opt = get_next_option(apc)) != -1) {
        switch(opt) {
            case 0: // Add module to list
                if(allModules || modList.empty())
                    modList.push_back(*get_option_arguments(apc));
                else
                    argumentList.push_back(*get_option_arguments(apc));
                break;
            case 'v':
                verbose = true;
                break;
            case 'a':
                allModules = true;
                break;
            case 'm':
                printMajor = true;
                break;
            case 'h':
            default:
                print_usage(argv[0]);
                return 0;
        }
    }

    load_module_deps();
    load_module_aliases();

    std::list<Module*> orderedModules;

    // Resolve all requested modules first
    for(auto& mod : modList) {
        Module* modStruct = probe(mod);
        if(!modStruct) {
            fprintf(stderr, "Module '%s' not found in '%s' directory\n", mod.c_str(), module_directory.c_str());
            return 1;
        }
        orderedModules.push_back(modStruct);
    }

    // Handle the dependencies of modules
    for(auto iter = orderedModules.begin(); iter != orderedModules.end(); ++iter) {
        Module* mod = *iter;
        handle_dependencies(mod, orderedModules, iter);
    }

    // Initialize the modules
    for(auto& mod : orderedModules) {
        if(verbose)
            fprintf(stderr, "insmod '%s'\n", mod->f_moduleFile.c_str());

        /// TODO: [02.02.2023] This will also handle arguments later.
        int major = insmod(mod->f_moduleFile, mod->f_moduleName);
        if(printMajor) {
            if(major > 0) {
                std::cout << mod->f_moduleName << "=" << major << std::endl;
            } else {
                std::cout << mod->f_moduleName << "=0" << std::endl;
            }
        }
    }

    return 0;
}

