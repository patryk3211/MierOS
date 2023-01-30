#ifndef LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_MIEROS_H
#define LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_MIEROS_H

#include "Gnu.h"
#include "clang/Driver/Tool.h"
#include "clang/Driver/ToolChain.h"

namespace clang {
namespace driver {
namespace toolchains {

namespace mieros {

class LLVM_LIBRARY_VISIBILITY Assemble : public Tool {
public:
  Assemble(const ToolChain &TC)
      : Tool("mieros::Assemble", "assembler", TC) { }

  bool hasIntegratedCPP() const override { return false; }

  void ConstructJob(Compilation &C, const JobAction &JA,
                    const InputInfo &Output,
                    const InputInfoList &Inputs,
                    const llvm::opt::ArgList &TCArgs,
                    const char *LinkingOutput) const override;
};

class LLVM_LIBRARY_VISIBILITY Link : public Tool {
public:
    Link(const ToolChain &TC)
        : Tool("mieros::Link", "linker", TC) { }

    bool hasIntegratedCPP() const override { return false; }
    bool isLinkJob() const override { return true; }

    void ConstructJob(Compilation &C, const JobAction &JA,
                    const InputInfo &Output,
                    const InputInfoList &Inputs,
                    const llvm::opt::ArgList &TCArgs,
                    const char *LinkingOutput) const override;
};

}

class LLVM_LIBRARY_VISIBILITY MierOSToolChain : public Generic_ELF {
public:
  MierOSToolChain(const Driver &D, const llvm::Triple &Triple,
                  const llvm::opt::ArgList &Args);

  /*bool HasNativeLLVMSupport() const override;

  void AddClangSystemIncludeArgs(const llvm::opt::ArgList &DriverArgs,
                                 llvm::opt::ArgStringList &CC1Args) const override;
  void AddLibStdCxxIncludePaths(const llvm::opt::ArgList &DriverArgs,
                                llvm::opt::ArgStringList &CC1Args) const override;*/
protected:
  Tool *buildAssembler() const override;
  Tool *buildLinker() const override;
};

}
}
}

#endif

