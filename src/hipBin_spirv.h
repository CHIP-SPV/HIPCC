/*
Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#ifndef SRC_HIPBIN_SPIRV_H_
#define SRC_HIPBIN_SPIRV_H_

#include "hipBin_base.h"
#include "hipBin_util.h"
#include <vector>
#include <string>
#include <unordered_set>
#include <cassert>

// Use (void) to silent unused warnings.
#define assertm(exp, msg) assert(((void)msg, exp))

/**
 *
 * # Auto-generated by cmake on 2022-09-01T08:07:28Z UTC
 * HIP_PATH=/home/pvelesko/space/install/HIP/clang14/chip-spv-0.9
 * HIP_RUNTIME=spirv
 * HIP_COMPILER=clang
 * HIP_ARCH=spirv
 * HIP_OFFLOAD_COMPILE_OPTIONS=-D__HIP_PLATFORM_SPIRV__= -x hip
 * --target=x86_64-linux-gnu --offload=spirv64 -nohipwrapperinc
 * --hip-path=/home/pvelesko/space/install/HIP/clang14/chip-spv-0.9
 * HIP_OFFLOAD_LINK_OPTIONS=-L/home/pvelesko/space/install/HIP/clang14/chip-spv-0.9/lib
 * -lCHIP -L/soft/libraries/khronos/loader/master-2022.05.18/lib64
 * -L/soft/restricted/CNDA/emb/libraries/intel-level-zero/api_+_loader/20220614.1/lib64
 * -lOpenCL -lze_loader
 * -Wl,-rpath,/home/pvelesko/space/install/HIP/clang14/chip-spv-0.9/lib
 */

#define HIP_OFFLOAD_COMPILE_OPTIONS "HIP_OFFLOAD_COMPILE_OPTIONS"
#define HIP_OFFLOAD_LINK_OPTIONS "HIP_OFFLOAD_LINK_OPTIONS"

/**
 * @brief Container class for parsing and storing .hipInfo
 *
 */
class HipInfo {
public:
  string runtime = "";
  string cxxflags = "";
  string ldflags = "";
  string clangpath = "";

  void parseLine(string line) {
    if (line.find(HIP_RUNTIME) != string::npos) {
      runtime = line.substr(string(HIP_RUNTIME).size() +
                            1); // add + 1 to account for =
    } else if (line.find(HIP_OFFLOAD_COMPILE_OPTIONS) != string::npos) {
      cxxflags = line.substr(string(HIP_OFFLOAD_COMPILE_OPTIONS).size() +
                             1); // add + 1 to account for =
    } else if (line.find(HIP_OFFLOAD_LINK_OPTIONS) != string::npos) {
      ldflags = line.substr(string(HIP_OFFLOAD_LINK_OPTIONS).size() +
                            1); // add + 1 to account for =
    } else if (line.find(HIP_CLANG_PATH) != string::npos) {
      // TODO check if llvm-config exists here
      clangpath = line.substr(string(HIP_CLANG_PATH).size() +
                              1); // add + 1 to account for =
    } else {
    }

    return;
  }
};

/**
 * @brief A container class for representing a single argument to hipcc
 *
 */
class Argument {
private:
  // Should this arg be passed onto clang++ invocation
  bool passthrough_ = true;

public:
  // Is the argument present/enabled
  bool present = false;
  // Any additional arguments to this argument. 
  // Example: "MyName" in `hipcc <...> -o MyName`
  string args;
  // Regex for which a match enables this argument
  regex regexp;

  Argument() {};
  Argument(string regexpIn) : regexp(regexpIn){};
  Argument(string regexpIn, bool passthrough)
      : regexp(regexpIn), passthrough_(passthrough){};
  /**
   * @brief Parse the compiler invocation and enable this argument
   *
   * @param argline string reprenting the campiler invocation
   */
  void parseLine(string argline) {
    smatch m;
    if (regex_search(argline, m, regexp)) {
      present = true;
      if (m.size() > 1) {
        // todo get more args
      }
    }

    if(present && !passthrough_) {
      regex_replace(argline, regexp, "");
    }
  }
};

class CompilerOptions {
public:
  int verbose = 0; // 0x1=commands, 0x2=paths, 0x4=hipcc args
  // bool setStdLib = 0; // set if user explicitly requests -stdlib=libc++
  Argument compile{"(\\s-c\\b|\\s\\S*cpp\\s)"}; // search for *.cpp src files or -c
  Argument compileOnly{"\\s-c\\b"}; // search for -c
  Argument outputObject{"\\s-o\\b"}; // search for -o 
  Argument needCXXFLAGS;  // need to add CXX flags to compile step
  Argument needCFLAGS;    // need to add C flags to compile step
  Argument needLDFLAGS;   // need to add LDFLAGS to compile step.
  Argument fileTypeFlag;  // to see if -x flag is mentioned
  Argument hasOMPTargets; // If OMP targets is mentioned
  Argument hasC;          // options contain a c-style file
  // options contain a cpp-style file (NVCC must force recognition as GPU
  // file)
  Argument hasCXX;
  // options contain a hip-style file (HIP-Clang must pass offloading options)
  Argument hasHIP;
  Argument printHipVersion{"\\s--short-version\\b", false}; // print HIP version
  Argument printCXXFlags{"\\s--cxxflags\\b", false};        // print HIPCXXFLAGS
  Argument printLDFlags{"\\s--ldflags\\b", false};          // print HIPLDFLAGS
  Argument runCmd;
  Argument buildDeps;
  Argument linkType;
  Argument setLinkType;
  Argument funcSupp; // enable function support
  Argument rdc;      // whether -fgpu-rdc is on

  void processArgs(vector<string> argv, EnvVariables var) {
    argv.erase(argv.begin()); // remove clang++
    string argStr;
    for (auto arg : argv)
      argStr += " " + arg;

    if (!var.verboseEnv_.empty())
      verbose = stoi(var.verboseEnv_);

    compile.parseLine(argStr);
    compileOnly.parseLine(argStr);
    outputObject.parseLine(argStr);

    printHipVersion.parseLine(argStr);
    printCXXFlags.parseLine(argStr);
    printLDFlags.parseLine(argStr);
    if (printHipVersion.present || printCXXFlags.present ||
        printLDFlags.present) {
      runCmd.present = false;
    } else {
      runCmd.present = true;
    }
  }
};
class HipBinSpirv : public HipBinBase {
private:
  HipBinUtil *hipBinUtilPtr_;
  string hipClangPath_ = "";
  PlatformInfo platformInfo_;
  string hipCFlags_, hipCXXFlags_, hipLdFlags_;
  HipInfo hipInfo_;

public:
  HipBinSpirv();
  virtual ~HipBinSpirv() = default;
  virtual bool detectPlatform();
  virtual void constructCompilerPath();
  virtual const string &getCompilerPath() const;
  virtual const PlatformInfo &getPlatformInfo() const;
  virtual string getCppConfig();
  virtual void printFull();
  virtual void printCompilerInfo() const;
  virtual string getCompilerVersion();
  virtual void checkHipconfig();
  virtual string getDeviceLibPath() const;
  virtual string getHipLibPath() const;
  virtual string getHipCC() const;
  virtual string getCompilerIncludePath();
  virtual string getHipInclude() const;
  virtual void initializeHipCXXFlags();
  virtual void initializeHipCFlags();
  virtual void initializeHipLdFlags();
  virtual const string &getHipCXXFlags() const;
  virtual const string &getHipCFlags() const;
  virtual const string &getHipLdFlags() const;
  virtual void executeHipCCCmd(vector<string> argv);

  bool readHipInfo(const string hip_path, HipInfo &result) {
    fs::path path(hip_path);
    path /= "share/.hipInfo";
    if (!fs::exists(path))
      return false;

    string hipInfoString;
    ifstream file(path);

    if (file.is_open()) {
      std::string line;
      while (std::getline(file, line)) {
        result.parseLine(line);
      }
      file.close();
    }

    return true;
  };
};

HipBinSpirv::HipBinSpirv() {
  PlatformInfo platformInfo;
  platformInfo.os = getOSInfo();

  platformInfo.platform = PlatformType::intel;
  platformInfo.runtime = RuntimeType::spirv;
  platformInfo.compiler = clang;
  platformInfo_ = platformInfo;

  return;
}

const string &HipBinSpirv::getHipCFlags() const { return hipCFlags_; }

const string &HipBinSpirv::getHipLdFlags() const { return hipLdFlags_; }

void HipBinSpirv::initializeHipLdFlags() {
  string hipLibPath;
  string hipLdFlags = hipInfo_.ldflags;
  // const string &hipClangPath = getCompilerPath();
  // // If $HIPCC clang++ is not compiled, use clang instead
  // string hipCC = "\"" + hipClangPath + "/clang++";

  // hipLibPath = getHipLibPath();
  // hipLdFlags += " -L\"" + hipLibPath + "\"";
  // const OsType &os = getOSInfo();

  hipLdFlags_ = hipLdFlags;
}

void HipBinSpirv::initializeHipCFlags() {
  string hipCFlags;
  // string hipclangIncludePath;
  // hipclangIncludePath = getHipInclude();
  // hipCFlags += " -isystem \"" + hipclangIncludePath + "\"";
  // const OsType &os = getOSInfo();

  string hipIncludePath;
  hipIncludePath = getHipInclude();
  hipCFlags += " -isystem \"" + hipIncludePath + "\"";
  hipCFlags_ = hipCFlags;
}

const string &HipBinSpirv::getHipCXXFlags() const { return hipCXXFlags_; }

string HipBinSpirv::getHipInclude() const {
  fs::path hipIncludefs = getHipPath();
  hipIncludefs /= "include";
  string hipInclude = hipIncludefs.string();
  return hipInclude;
}

void HipBinSpirv::initializeHipCXXFlags() {
  string hipCXXFlags = hipInfo_.cxxflags;
  // const OsType &os = getOSInfo();
  // string hipClangIncludePath;
  // hipClangIncludePath = getCompilerIncludePath();
  // hipCXXFlags += " -isystem \"" + hipClangIncludePath;
  // fs::path hipCXXFlagsTempFs = hipCXXFlags;
  // hipCXXFlagsTempFs /= "..\"";
  // hipCXXFlags = hipCXXFlagsTempFs.string();
  // const EnvVariables &var = getEnvVariables();
  // // Allow __fp16 as function parameter and return type.
  // if (var.hipClangHccCompactModeEnv_.compare("1") == 0) {
  //   hipCXXFlags += " -Xclang -fallow-half-arguments-and-returns "
  //                  "-D__HIP_HCC_COMPAT_MODE__=1";
  // }

  // Add paths to common HIP includes:
  string hipIncludePath;
  hipIncludePath = getHipInclude();
  hipCXXFlags += " -isystem \"" + hipIncludePath + "\"";
  hipCXXFlags_ = hipCXXFlags;
}

// populates clang path.
void HipBinSpirv::constructCompilerPath() {
  string compilerPath = "";
  const EnvVariables &envVariables = getEnvVariables();

  // first check if HIP_CLANG_PATH is set and if so, then make sure llvm-config
  // can be found. This way user overrides always take precedence and get error
  // checked
  fs::path llvmPath = envVariables.hipClangPathEnv_;
  if (!llvmPath.empty()) {
    llvmPath /= "llvm-config";
    if (!fs::exists(llvmPath)) {
      cout << "Error: HIP_CLANG_PATH was set in the environment "
              "HIP_CLANG_PATH="
           << envVariables.hipClangPathEnv_
           << " but llvm-config was not found in " << llvmPath << endl;
      std::exit(EXIT_FAILURE);
    } else {
      hipClangPath_ = envVariables.hipClangPathEnv_;
      return;
    }

  } else {
    hipClangPath_ = hipInfo_.clangpath;
  }

  return;
}

// returns clang path.
const string &HipBinSpirv::getCompilerPath() const { return hipClangPath_; }

void HipBinSpirv::printCompilerInfo() const {
  const string &hipClangPath = getCompilerPath();

  cout << endl;

  string cmd = hipClangPath + "/clang++ --version";
  system(cmd.c_str()); // hipclang version
  cmd = hipClangPath + "/llc --version";
  system(cmd.c_str()); // llc version
  cout << "hip-clang-cxxflags :" << endl;
  cout << hipInfo_.cxxflags << endl;

  cout << endl << "hip-clang-ldflags :" << endl;
  cout << hipInfo_.ldflags << endl;

  cout << endl;
}

string HipBinSpirv::getCompilerVersion() {
  string out, complierVersion;
  const string &hipClangPath = getCompilerPath();
  fs::path cmd = hipClangPath;
  /**
   * Ubuntu systems do not provide this symlink clang++ -> clang++-14
   * Maybe use llvm-config instead?
   * $: llvm-config --version
   * $: 14.0.0
   */
  cmd /= "/llvm-config";
  if (canRunCompiler(cmd.string(), out) || canRunCompiler("llvm-config", out)) {
    regex regexp("([0-9.]+)");
    smatch m;
    if (regex_search(out, m, regexp)) {
      if (m.size() > 1) {
        // get the index =1 match, 0=whole match we ignore
        std::ssub_match sub_match = m[1];
        complierVersion = sub_match.str();
      }
    }
  } else {
    cout << "Hip Clang Compiler not found" << endl;
  }
  return complierVersion;
}

const PlatformInfo &HipBinSpirv::getPlatformInfo() const {
  return platformInfo_;
}

string HipBinSpirv::getCppConfig() { return hipInfo_.cxxflags; }

string HipBinSpirv::getDeviceLibPath() const { return ""; }

bool HipBinSpirv::detectPlatform() {
  if (getOSInfo() == windows) {
    return false;
  }

  bool detected = false;
  const EnvVariables &var = getEnvVariables();

  /**
   * 1. Check if .hipInfo is available in the ../share directory as I am
   *     a. Check .hipInfo for HIP_PLATFORM==spirv if so, return detected
   * 2. If HIP_PATH is set, check ${HIP_PATH}/share for .hipInfo
   *     b. If .hipInfo HIP_PLATFORM=spirv, return detected
   * 3. We haven't found .hipVars meaning we couldn't find a good installation
   * of CHIP-SPV. Check to see if the environment variables were not forcing
   * spirv and if so, return an error explaining what went wrong.
   */

  HipInfo hipInfo;
  fs::path currentBinaryPath = fs::canonical("/proc/self/exe");
  currentBinaryPath = currentBinaryPath.parent_path();
  fs::path sharePath = currentBinaryPath.string() + "/../share";
  if (readHipInfo( // 1.
          sharePath, hipInfo)) {
    detected = hipInfo.runtime.compare("spirv") == 0; // b.
    hipInfo_ = hipInfo;
  } else if (!var.hipPathEnv_.empty()) { // 2.
    if (readHipInfo(var.hipPathEnv_, hipInfo)) {
      detected = hipInfo.runtime == "spirv";

      // check that HIP_RUNTIME found in .hipVars does not conflict with
      // HIP_RUNTIME in the env
      if (detected && !var.hipPlatformEnv_.empty()) {
        if (var.hipPlatformEnv_ != "spirv" || var.hipPlatformEnv_ != "intel") {
          cout << "Error: .hipVars was found in " << var.hipPathEnv_
               << " where HIP_PLATFORM=spirv which conflicts with HIP_PLATFORM "
                  "set in the current environment where HIP_PLATFORM="
               << var.hipPlatformEnv_ << endl;
          std::exit(EXIT_FAILURE);
        }
      }

      hipInfo_ = hipInfo;
    }
  }

  if (!detected && (var.hipPlatformEnv_ == "spirv" || var.hipPlatformEnv_ == "intel")) { // 3.
    if (var.hipPathEnv_.empty()) {
      cout << "Error: setting HIP_PLATFORM=spirv/intel requires setting "
              "HIP_PATH=/path/to/CHIP-SPV-INSTALL-DIR/"
           << endl;
      std::exit(EXIT_FAILURE);
    } else {
      cout
          << "Error: HIP_PLATFORM=" << var.hipPlatformEnv_
          << " was set but .hipInfo(generated during CHIP-SPV install) was not "
             "found in HIP_PATH="
          << var.hipPathEnv_ << "/share" << endl;
      std::exit(EXIT_FAILURE);
    }
  }

  // Because our compiler path gets read from hipInfo, we need to do this here
  constructCompilerPath();
  return detected;
}

string HipBinSpirv::getHipLibPath() const { return ""; }

string HipBinSpirv::getHipCC() const {
  string hipCC;
  const string &hipClangPath = getCompilerPath();
  fs::path compiler = hipClangPath;
  compiler /= "clang++";
  if (!fs::exists(compiler)) {
    fs::path compiler = hipClangPath;
    compiler /= "clang";
  }
  hipCC = compiler.string();
  return hipCC;
}

string HipBinSpirv::getCompilerIncludePath() {
  string hipClangVersion, includePath, compilerIncludePath;
  const string &hipClangPath = getCompilerPath();
  hipClangVersion = getCompilerVersion();
  fs::path includePathfs = hipClangPath;
  includePathfs = includePathfs.parent_path();
  includePathfs /= "lib/clang/";
  includePathfs /= hipClangVersion;
  includePathfs /= "include";
  includePathfs = fs::absolute(includePathfs).string();
  compilerIncludePath = includePathfs.string();
  return compilerIncludePath;
}

void HipBinSpirv::checkHipconfig() {
  printFull();
  cout << endl << "Check system installation: " << endl;
  cout << "check hipconfig in PATH..." << endl;
  if (system("which hipconfig > /dev/null 2>&1") != 0) {
    cout << "FAIL " << endl;
  } else {
    cout << "good" << endl;
  }
  string ldLibraryPath;
  const EnvVariables &env = getEnvVariables();
  ldLibraryPath = env.ldLibraryPathEnv_;
  const string &hsaPath(""); // = getHsaPath();
  cout << "check LD_LIBRARY_PATH (" << ldLibraryPath << ") contains HSA_PATH ("
       << hsaPath << ")..." << endl;
  if (ldLibraryPath.find(hsaPath) == string::npos) {
    cout << "FAIL" << endl;
  } else {
    cout << "good" << endl;
  }
}

void HipBinSpirv::printFull() {
  const string &hipVersion = getHipVersion();
  const string &hipPath = getHipPath();
  const PlatformInfo &platformInfo = getPlatformInfo();
  const string &ccpConfig = getCppConfig();
  const string &hsaPath(""); // = getHsaPath();
  const string &hipClangPath = getCompilerPath();

  cout << "HIP version: " << hipVersion << endl;
  cout << endl << "==hipconfig" << endl;
  cout << "HIP_PATH           :" << hipPath << endl;
  cout << "HIP_COMPILER       :" << CompilerTypeStr(platformInfo.compiler)
       << endl;
  cout << "HIP_PLATFORM       :" << PlatformTypeStr(platformInfo.platform)
       << endl;
  cout << "HIP_RUNTIME        :" << RuntimeTypeStr(platformInfo.runtime)
       << endl;
  cout << "CPP_CONFIG         :" << ccpConfig << endl;

  cout << endl << "==hip-clang" << endl;
  cout << "HIP_CLANG_PATH     :" << hipClangPath << endl;
  printCompilerInfo();
  cout << endl << "== Envirnoment Variables" << endl;
  printEnvironmentVariables();
  getSystemInfo();
  if (fs::exists("/usr/bin/lsb_release"))
    system("/usr/bin/lsb_release -a");
  cout << endl;
}

void HipBinSpirv::executeHipCCCmd(vector<string> origArgv) {
  vector<string> argv;
  // Filter away a possible duplicate --offload=spirv64 flag which can be passed
  // in builds that call hipcc with direct output of hip_config --cpp_flags.
  // We have to include the flag in the --cpp_flags output to retain the
  // option to use clang++ directly for HIP compilation instead of hipcc.
  for (int i = 0; i < origArgv.size(); i++) {
    if (origArgv[i] != "--offload=spirv64")
      argv.push_back(origArgv[i]);
  }

  if (argv.size() < 2) {
    cout << "No Arguments passed, exiting ...\n";
    exit(EXIT_SUCCESS);
  }

  CompilerOptions opts;
  EnvVariables var = getEnvVariables();
  opts.processArgs(argv, var);

  string toolArgs; // arguments to pass to the clang tool

  const OsType &os = getOSInfo();
  string hip_compile_cxx_as_hip;
  if (var.hipCompileCxxAsHipEnv_.empty()) {
    hip_compile_cxx_as_hip = "1";
  } else {
    hip_compile_cxx_as_hip = var.hipCompileCxxAsHipEnv_;
  }

  string HIPLDARCHFLAGS;

  initializeHipCXXFlags();
  initializeHipCFlags();
  initializeHipLdFlags();
  string HIPCXXFLAGS, HIPCFLAGS, HIPLDFLAGS;
  HIPCFLAGS = getHipCFlags();
  HIPCXXFLAGS = getHipCXXFlags();
  HIPLDFLAGS = getHipLdFlags();
  string hipLibPath;
  string hipclangIncludePath, hipIncludePath, deviceLibPath;
  hipLibPath = getHipLibPath();
  const string &roccmPath = getRoccmPath();
  const string &hipPath = getHipPath();
  const PlatformInfo &platformInfo = getPlatformInfo();
  const string &rocclrHomePath(""); // = getRocclrHomePath();
  const string &hipClangPath = getCompilerPath();
  hipclangIncludePath = getCompilerIncludePath();
  hipIncludePath = getHipInclude();
  deviceLibPath = getDeviceLibPath();
  const string &hipVersion = getHipVersion();
  if (opts.verbose & 0x2) {
    cout << "HIP_PATH=" << hipPath << endl;
    cout << "HIP_PLATFORM=" << PlatformTypeStr(platformInfo.platform) << endl;
    cout << "HIP_COMPILER=" << CompilerTypeStr(platformInfo.compiler) << endl;
    cout << "HIP_RUNTIME=" << RuntimeTypeStr(platformInfo.runtime) << endl;
    cout << "HIP_CLANG_PATH=" << hipClangPath << endl;
    cout << "HIP_CLANG_INCLUDE_PATH=" << hipclangIncludePath << endl;
    cout << "HIP_INCLUDE_PATH=" << hipIncludePath << endl;
    cout << "HIP_LIB_PATH=" << hipLibPath << endl;
    cout << "DEVICE_LIB_PATH=" << deviceLibPath << endl;
  }

  if (opts.verbose & 0x4) {
    cout << "hipcc-args: ";
    for (unsigned int i = 1; i < argv.size(); i++) {
      cout << argv.at(i) << " ";
    }
    cout << endl;
  }

  // Add --hip-link only if it is compile only and -fgpu-rdc is on.
  if (opts.rdc.present && !opts.compileOnly.present) {
    HIPLDFLAGS += " --hip-link";
    HIPLDFLAGS += HIPLDARCHFLAGS;
  }

  // if (opts.hasHIP) {
  //   fs::path bitcodeFs = roccmPath;
  //   bitcodeFs /= "amdgcn/bitcode";
  //   if (deviceLibPath != bitcodeFs.string()) {
  //     string hip_device_lib_str =
  //         " --hip-device-lib-path=\"" + deviceLibPath + "\"";
  //     HIPCXXFLAGS += hip_device_lib_str;
  //   }
  // }
  // if (os != windows) {
  //   HIPLDFLAGS += " -lgcc_s -lgcc -lpthread -lm -lrt";
  // }

  // if (os != windows && !opts.compileOnly) {
  //   string hipClangVersion;

  //   hipClangVersion = getCompilerVersion();
  //   // To support __fp16 and _Float16, explicitly link with compiler-rt
  //   toolArgs += " -L" + hipClangPath + "/../lib/clang/" + hipClangVersion +
  //               "/lib/linux -lclang_rt.builtins-x86_64 ";
  // }
  if (!var.hipccCompileFlagsAppendEnv_.empty()) {
    HIPCXXFLAGS += " " + var.hipccCompileFlagsAppendEnv_ + " ";
    HIPCFLAGS += " " + var.hipccCompileFlagsAppendEnv_ + " ";
  }
  if (!var.hipccLinkFlagsAppendEnv_.empty()) {
    HIPLDFLAGS += " " + var.hipccLinkFlagsAppendEnv_ + " ";
  }
  // TODO(hipcc): convert CMD to an array rather than a string
  string compiler;
  compiler = getHipCC();
  string CMD = compiler;

  if (opts.needCFLAGS.present) {
    CMD += " " + HIPCFLAGS;
  }

  opts.needCXXFLAGS.present = opts.compile.present;
  if (opts.needCXXFLAGS.present) {
    CMD += " " + HIPCXXFLAGS;
  }

  opts.needLDFLAGS.present =
      opts.outputObject.present && !opts.compileOnly.present;
  if (opts.needLDFLAGS.present) {
    CMD += " " + HIPLDFLAGS;
  }

  CMD += " " + toolArgs;

  if (opts.printHipVersion.present) {
    if (opts.runCmd.present) {
      cout << "HIP version: ";
    }
    cout << hipVersion << endl;
  }
  if (opts.printCXXFlags.present) {
    cout << HIPCXXFLAGS;
  }
  if (opts.printLDFlags.present) {
    cout << HIPLDFLAGS;
  }

  // 1st arg is the full path to hipcc
  argv.erase(argv.begin());
  for (auto arg : argv) {
    CMD += " " + arg;
  }

  if (opts.verbose & 0x1) {
    cout << "hipcc-cmd: " << CMD << "\n";
  }

  if (opts.runCmd.present) {
    SystemCmdOut sysOut;
    sysOut = hipBinUtilPtr_->exec(CMD.c_str(), true);
    string cmdOut = sysOut.out;
    int CMD_EXIT_CODE = sysOut.exitCode;
    if (CMD_EXIT_CODE != 0) {
      cout << "failed to execute:" << CMD << std::endl;
    }
    exit(CMD_EXIT_CODE);
  } // end of runCmd section
} // end of function

#endif // SRC_HIPBIN_SPIRV_H_
