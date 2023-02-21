set(target "x86_64-pc-none-elf")

set(CMAKE_INSTALL_PREFIX "${CROSS_PATH}/kern-llvm" CACHE STRING "")
set(LLVM_TABLEGEN "${CROSS_PATH}/llvm-host-tbl-build/bin/llvm-tblgen" CACHE STRING "")
set(CLANG_TABLEGEN "${CROSS_PATH}/llvm-host-tbl-build/bin/clang-tblgen" CACHE STRING "")
set(LLVM_DEFAULT_TARGET_TRIPLE ${target} CACHE STRING "")
set(LLVM_TARGET_ARCH "X86" CACHE STRING "")
set(LLVM_TARGETS_TO_BUILD "x86_64" CACHE STRING "")
set(LLVM_ENABLE_PROJECTS "clang;lld" CACHE STRING "")
set(LLVM_ENABLE_RUNTIMES "compiler-rt" CACHE STRING "")
set(CMAKE_BUILD_TYPE Release CACHE STRING "")

