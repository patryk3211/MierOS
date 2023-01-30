#include "MierOS.h"
#include "CommonArgs.h"
#include "clang/Driver/Compilation.h"
#include "clang/Driver/Driver.h"
#include "clang/Driver/Job.h"
#include <clang/Driver/Options.h>

using namespace llvm::opt;
using namespace clang::driver;
using namespace clang::driver::toolchains;
using namespace clang::driver::tools;

MierOSToolChain::MierOSToolChain(const Driver &D, const llvm::Triple &Triple,
               const llvm::opt::ArgList &Args)
    : Generic_ELF(D, Triple, Args) {
}

Tool *MierOSToolChain::buildAssembler() const {
  return new mieros::Assemble(*this);
}

Tool *MierOSToolChain::buildLinker() const {
  return new mieros::Link(*this);
}

void mieros::Assemble::ConstructJob(Compilation &C, const JobAction &JA,
                                    const InputInfo &Output,
                                    const InputInfoList &Inputs,
                                    const ArgList &Args,
                                    const char *LinkingOutput) const {
  ArgStringList CmdArgs;
 
  Args.AddAllArgValues(CmdArgs, options::OPT_Wa_COMMA, options::OPT_Xassembler);
 
  CmdArgs.push_back("-o");
  CmdArgs.push_back(Output.getFilename());
 
  for (const auto &II : Inputs)
    CmdArgs.push_back(II.getFilename());
 
  const char *Exec = Args.MakeArgString(getToolChain().GetProgramPath("as"));
  C.addCommand(std::make_unique<Command>(JA, *this, ResponseFileSupport::AtFileCurCP(), Exec, CmdArgs, Inputs, Output));
}
 
void mieros::Link::ConstructJob(Compilation &C, const JobAction &JA,
                              const InputInfo &Output,
                              const InputInfoList &Inputs,
                              const ArgList &Args,
                              const char *LinkingOutput) const {
  const Driver &D = getToolChain().getDriver();
  ArgStringList CmdArgs;
 
  if (Output.isFilename()) {
    CmdArgs.push_back("-o");
    CmdArgs.push_back(Output.getFilename());
  } else {
    assert(Output.isNothing() && "Invalid output.");
  }
 
  if (!Args.hasArg(options::OPT_nostdlib, options::OPT_nostartfiles)) {
      CmdArgs.push_back(Args.MakeArgString(getToolChain().GetFilePath("crt1.o")));
      CmdArgs.push_back(Args.MakeArgString(getToolChain().GetFilePath("crti.o")));
      CmdArgs.push_back(Args.MakeArgString(getToolChain().GetFilePath("crtbegin.o")));
      CmdArgs.push_back(Args.MakeArgString(getToolChain().GetFilePath("crtn.o")));
  }
 
  Args.AddAllArgs(CmdArgs, options::OPT_L);
  Args.AddAllArgs(CmdArgs, options::OPT_T_Group);
  Args.AddAllArgs(CmdArgs, options::OPT_e);
 
  AddLinkerInputs(getToolChain(), Inputs, Args, CmdArgs, JA);
 
  getToolChain().addProfileRTLibs(Args, CmdArgs);
 
  if (!Args.hasArg(options::OPT_nostdlib, options::OPT_nodefaultlibs)) {
    if (D.CCCIsCXX()) {
      getToolChain().AddCXXStdlibLibArgs(Args, CmdArgs);
      CmdArgs.push_back("-lm");
    }
  }

  if (!Args.hasArg(options::OPT_nostdlib, options::OPT_nostartfiles)) {
    CmdArgs.push_back("-lc");
    CmdArgs.push_back("-lCompilerRT-Generic");
    CmdArgs.push_back("-L/usr/pkg/compiler-rt/lib");
    CmdArgs.push_back(Args.MakeArgString(getToolChain().GetFilePath("crtend.o")));
  }
 
  const char *Exec = Args.MakeArgString(getToolChain().GetLinkerPath());
  C.addCommand(std::make_unique<Command>(JA, *this, ResponseFileSupport::AtFileCurCP(), Exec, CmdArgs, Inputs, Output));
}

