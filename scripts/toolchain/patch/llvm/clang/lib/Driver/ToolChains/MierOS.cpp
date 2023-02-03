#include "MierOS.h"
#include "CommonArgs.h"
#include "clang/Config/config.h"
#include "clang/Driver/Compilation.h"
#include "clang/Driver/Driver.h"
#include "clang/Driver/Job.h"
#include "clang/Driver/Options.h"
#include "clang/Basic/DiagnosticDriver.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/VirtualFileSystem.h"

using namespace llvm::opt;
using namespace clang::driver;
using namespace clang::driver::toolchains;
using namespace clang::driver::tools;

MierOSToolChain::MierOSToolChain(const Driver &D, const llvm::Triple &Triple,
               const llvm::opt::ArgList &Args)
    : Generic_ELF(D, Triple, Args) {
  getProgramPaths().push_back(D.getInstalledDir());
  if (D.getInstalledDir() != D.Dir)
    getProgramPaths().push_back(D.Dir);

  if (!D.SysRoot.empty()) {
    SmallString<128> P(D.SysRoot);
    llvm::sys::path::append(P, "lib");
    getFilePaths().push_back(std::string(P.str()));
  }
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
  const toolchains::MierOSToolChain &ToolChain = static_cast<const toolchains::MierOSToolChain &>(getToolChain());
  const Driver &D = getToolChain().getDriver();
  ArgStringList CmdArgs;

  Args.ClaimAllArgs(options::OPT_g_Group);
  Args.ClaimAllArgs(options::OPT_emit_llvm);
  Args.ClaimAllArgs(options::OPT_w);

  CmdArgs.push_back("-z");
  CmdArgs.push_back("max-page-size=4096");

  if (!D.SysRoot.empty())
    CmdArgs.push_back(Args.MakeArgString("--sysroot=" + D.SysRoot));
  if (!Args.hasArg(options::OPT_shared) && !Args.hasArg(options::OPT_r))
    CmdArgs.push_back("-pie");
  if (Args.hasArg(options::OPT_rdynamic))
    CmdArgs.push_back("-export-dynamic");
  
  if (Args.hasArg(options::OPT_s))
    CmdArgs.push_back("-s");
  if (Args.hasArg(options::OPT_r))
    CmdArgs.push_back("-r");
  else {
    CmdArgs.push_back("--build-id");
    CmdArgs.push_back("--hash-style=gnu");
  }

  if (Args.hasArg(options::OPT_shared))
    CmdArgs.push_back("-shared");
  else if (Args.hasArg(options::OPT_static))
    CmdArgs.push_back("-Bstatic");


  if(Output.isFilename()) {
    CmdArgs.push_back("-o");
    CmdArgs.push_back(Output.getFilename());
  } else {
    assert(Output.isNothing() && "Invalid output.");
  }

  Args.AddAllArgs(CmdArgs, options::OPT_L);
  Args.AddAllArgs(CmdArgs, options::OPT_u);

  ToolChain.AddFilePathLibArgs(Args, CmdArgs);

  if(D.isUsingLTO()) {
    assert(!Inputs.empty() && "Must have at least one input.");
    addLTOOptions(ToolChain, Args, CmdArgs, Output, Inputs[0], D.getLTOMode() == LTOK_Thin);
  }

  addLinkerCompressDebugSectionsOption(ToolChain, Args, CmdArgs);
  AddLinkerInputs(ToolChain, Inputs, Args, CmdArgs, JA);

  if (!Args.hasArg(options::OPT_r, options::OPT_nostdlib, options::OPT_nodefaultlibs)) {
    if (Args.hasArg(options::OPT_static))
      CmdArgs.push_back("-Bdynamic");

    if (!Args.hasArg(options::OPT_nostartfiles))
      CmdArgs.push_back(Args.MakeArgString(ToolChain.GetFilePath("crt0.o")));

    if (D.CCCIsCXX()) {
      if (ToolChain.ShouldLinkCXXStdlib(Args)) {
        bool OnlyLibstdcxxStatic = Args.hasArg(options::OPT_static_libstdcxx) && !Args.hasArg(options::OPT_static);
        CmdArgs.push_back("--push-state");
        CmdArgs.push_back("--as-needed");
        if (OnlyLibstdcxxStatic)
          CmdArgs.push_back("-Bstatic");
        ToolChain.AddCXXStdlibLibArgs(Args, CmdArgs);
        if (OnlyLibstdcxxStatic)
          CmdArgs.push_back("-Bdynamic");
        CmdArgs.push_back("-lm");
        CmdArgs.push_back("--pop-state");
      }
    }

    AddRunTimeLibs(ToolChain, D, CmdArgs, Args);

    if (!Args.hasArg(options::OPT_nolibc))
      CmdArgs.push_back("-lc");
  }

      //  if (Output.isFilename()) {
 //    CmdArgs.push_back("-o");
 //    CmdArgs.push_back(Output.getFilename());
 //  } else {
 //    assert(Output.isNothing() && "Invalid output.");
 //  }
 // 
 //  if (!Args.hasArg(options::OPT_nostdlib, options::OPT_nostartfiles)) {
 //      // CmdArgs.push_back(Args.MakeArgString(getToolChain().GetFilePath("crt1.o")));
 //      // CmdArgs.push_back(Args.MakeArgString(getToolChain().GetFilePath("crti.o")));
 //      // CmdArgs.push_back(Args.MakeArgString(getToolChain().GetFilePath("crtbegin.o")));
 //      // CmdArgs.push_back(Args.MakeArgString(getToolChain().GetFilePath("crtn.o")));
 //  }
 // 
 //  Args.AddAllArgs(CmdArgs, options::OPT_L);
 //  Args.AddAllArgs(CmdArgs, options::OPT_T_Group);
 //  Args.AddAllArgs(CmdArgs, options::OPT_e);
 // 
 //  AddLinkerInputs(getToolChain(), Inputs, Args, CmdArgs, JA);
 // 
 //  getToolChain().addProfileRTLibs(Args, CmdArgs);
 // 
 //  if (!Args.hasArg(options::OPT_nostdlib, options::OPT_nodefaultlibs)) {
 //    if (D.CCCIsCXX()) {
 //      getToolChain().AddCXXStdlibLibArgs(Args, CmdArgs);
 //      CmdArgs.push_back("-lm");
 //    }
 //  }
 //
 //  if (!Args.hasArg(options::OPT_nostdlib, options::OPT_nostartfiles)) {
 //    CmdArgs.push_back("-lc");
 //    CmdArgs.push_back("-lCompilerRT-Generic");
 //    CmdArgs.push_back("-L/usr/pkg/compiler-rt/lib");
 //    // CmdArgs.push_back(Args.MakeArgString(getToolChain().GetFilePath("crtend.o")));
 //  }
 // 
  const char *Exec = Args.MakeArgString(getToolChain().GetLinkerPath());
  C.addCommand(std::make_unique<Command>(JA, *this, ResponseFileSupport::AtFileCurCP(), Exec, CmdArgs, Inputs, Output));
}

ToolChain::RuntimeLibType MierOSToolChain::GetDefaultRuntimeLibType() const {
  return ToolChain::RLT_CompilerRT;
}

ToolChain::CXXStdlibType MierOSToolChain::GetDefaultCXXStdlibType() const {
  return ToolChain::CST_Libcxx;
}

ToolChain::RuntimeLibType MierOSToolChain::GetRuntimeLibType(const llvm::opt::ArgList &Args) const {
  if (Arg* A = Args.getLastArg(clang::driver::options::OPT_rtlib_EQ)) {
    StringRef Value = A->getValue();
    if(Value != "compiler-rt")
      getDriver().Diag(clang::diag::err_drv_invalid_rtlib_name) << A->getAsString(Args);
  }

  return ToolChain::RLT_CompilerRT;
}

ToolChain::CXXStdlibType MierOSToolChain::GetCXXStdlibType(const llvm::opt::ArgList &Args) const {
  if (Arg* A = Args.getLastArg(clang::driver::options::OPT_stdlib_EQ)) {
    StringRef Value = A->getValue();
    if(Value != "libc++")
      getDriver().Diag(clang::diag::err_drv_invalid_stdlib_name) << A->getAsString(Args);
  }

  return ToolChain::CST_Libcxx;
}

ToolChain::UnwindLibType MierOSToolChain::GetUnwindLibType(const llvm::opt::ArgList &Args) const {
  if (Arg* A = Args.getLastArg(clang::driver::options::OPT_unwindlib_EQ)) {
    StringRef Value = A->getValue();
    if(Value == "none")
      return ToolChain::UNW_None;
    else if(Value == "" || Value == "platform" || Value == "libunwind")
      return ToolChain::UNW_CompilerRT;
    else
      getDriver().Diag(clang::diag::err_drv_invalid_unwindlib_name) << A->getAsString(Args);
  }

  return ToolChain::UNW_CompilerRT;
}

void MierOSToolChain::AddCXXStdlibLibArgs(const ArgList &Args, ArgStringList &CmdArgs) const {
  switch (GetCXXStdlibType(Args)) {
    case ToolChain::CST_Libcxx:
      CmdArgs.push_back("-lc++");
      if (Args.hasArg(options::OPT_fexperimental_library))
        CmdArgs.push_back("-lc++experimental");
      break;
    case ToolChain::CST_Libstdcxx:
      llvm_unreachable("invalid stdlib name");
  }
}

void MierOSToolChain::AddClangSystemIncludeArgs(const ArgList &DriverArgs, ArgStringList &CC1Args) const {
  const Driver &D = getDriver();

  if (DriverArgs.hasArg(options::OPT_nostdinc))
    return;

  if (!DriverArgs.hasArg(options::OPT_nobuiltininc)) {
    SmallString<128> P(D.ResourceDir);
    llvm::sys::path::append(P, "include");
    addSystemInclude(DriverArgs, CC1Args, P);
  }

  if (DriverArgs.hasArg(options::OPT_nostdlibinc))
    return;

  StringRef CIncludeDirs(C_INCLUDE_DIRS);
  if (CIncludeDirs != "") {
    SmallVector<StringRef, 5> dirs;
    CIncludeDirs.split(dirs, ":");
    for (StringRef dir : dirs) {
      StringRef Prefix =
          llvm::sys::path::is_absolute(dir) ? "" : StringRef(D.SysRoot);
      addExternCSystemInclude(DriverArgs, CC1Args, Prefix + dir);
    }
    return;
  }

  if (!D.SysRoot.empty()) {
    SmallString<128> P(D.SysRoot);
    llvm::sys::path::append(P, "include");
    addExternCSystemInclude(DriverArgs, CC1Args, P.str());
  }
}

void MierOSToolChain::AddClangCXXStdlibIncludeArgs(const ArgList &DriverArgs, ArgStringList &CC1Args) const {
  if (DriverArgs.hasArg(options::OPT_nostdlibinc) ||
      DriverArgs.hasArg(options::OPT_nostdincxx) ||
      DriverArgs.hasArg(options::OPT_nostdinc))
    return;

  const Driver &D = getDriver();
  std::string Target = getTripleString();

  auto AddCXXIncludePath = [&](StringRef Path) {
    std::string Version = detectLibcxxVersion(Path);
    if (Version.empty())
      return;

    SmallString<128> TargetDir(Path);
    llvm::sys::path::append(TargetDir, Target, "c++", Version);
    if (getVFS().exists(TargetDir))
      addSystemInclude(DriverArgs, CC1Args, TargetDir);

    SmallString<128> Dir(Path);
    llvm::sys::path::append(Dir, "c++", Version);
    addSystemInclude(DriverArgs, CC1Args, Dir);
  };

  switch (GetCXXStdlibType(DriverArgs)) {
  case ToolChain::CST_Libcxx: {
    SmallString<128> P(D.Dir);
    llvm::sys::path::append(P, "..", "include");
    AddCXXIncludePath(P);
    break;
  }
  default:
    llvm_unreachable("invalid stdlib name");
  }
}

const char *MierOSToolChain::getDefaultLinker() const {
  return "ld.lld";
}

