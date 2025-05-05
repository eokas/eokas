
set(EOKAS_TARGET_NAME "elang")
set(EOKAS_TARGET_DIR "${EOKAS_MODULES_DIR}/${EOKAS_TARGET_NAME}")

message("EOKAS_TARGET_NAME = ${EOKAS_TARGET_NAME}")
message("EOKAS_TARGET_DIR = ${EOKAS_TARGET_DIR}")


set(EOKAS_HEADER_DIRS
        $ENV{LLVM_SDK_PATH}/include
        "${EOKAS_MODULES_DIR}"
)

set(EOKAS_LIBRARY_DIRS
        $ENV{LLVM_SDK_PATH}/lib
)

#set(LLVM_DEPS m z curses xml2)
set(LLVM_DEPS "")
set(LLVM_LIBS
        LLVMWindowsManifest LLVMXRay LLVMLibDriver LLVMDlltoolDriver LLVMCoverage LLVMLineEditor LLVMXCoreDisassembler
        LLVMXCoreCodeGen LLVMXCoreDesc LLVMXCoreInfo LLVMX86Disassembler LLVMX86AsmParser LLVMX86CodeGen
        LLVMX86Desc LLVMX86Info LLVMWebAssemblyDisassembler LLVMWebAssemblyAsmParser LLVMWebAssemblyCodeGen
        LLVMWebAssemblyDesc LLVMWebAssemblyInfo LLVMSystemZDisassembler LLVMSystemZAsmParser LLVMSystemZCodeGen
        LLVMSystemZDesc LLVMSystemZInfo LLVMSparcDisassembler LLVMSparcAsmParser LLVMSparcCodeGen LLVMSparcDesc
        LLVMSparcInfo LLVMRISCVDisassembler LLVMRISCVAsmParser LLVMRISCVCodeGen LLVMRISCVDesc LLVMRISCVInfo
        LLVMPowerPCDisassembler LLVMPowerPCAsmParser LLVMPowerPCCodeGen LLVMPowerPCDesc LLVMPowerPCInfo
        LLVMNVPTXCodeGen LLVMNVPTXDesc LLVMNVPTXInfo LLVMMSP430Disassembler LLVMMSP430AsmParser
        LLVMMSP430CodeGen LLVMMSP430Desc LLVMMSP430Info LLVMMipsDisassembler LLVMMipsAsmParser LLVMMipsCodeGen
        LLVMMipsDesc LLVMMipsInfo LLVMLanaiDisassembler LLVMLanaiCodeGen LLVMLanaiAsmParser LLVMLanaiDesc
        LLVMLanaiInfo LLVMHexagonDisassembler LLVMHexagonCodeGen LLVMHexagonAsmParser LLVMHexagonDesc LLVMHexagonInfo
        LLVMBPFDisassembler LLVMBPFAsmParser LLVMBPFCodeGen LLVMBPFDesc LLVMBPFInfo LLVMAVRDisassembler LLVMAVRAsmParser
        LLVMAVRCodeGen LLVMAVRDesc LLVMAVRInfo LLVMARMDisassembler LLVMARMAsmParser LLVMARMCodeGen LLVMARMDesc
        LLVMARMUtils LLVMARMInfo LLVMAMDGPUDisassembler LLVMAMDGPUAsmParser LLVMAMDGPUCodeGen LLVMAMDGPUDesc
        LLVMAMDGPUUtils LLVMAMDGPUInfo LLVMAArch64Disassembler LLVMAArch64AsmParser LLVMAArch64CodeGen LLVMAArch64Desc
        LLVMAArch64Utils LLVMAArch64Info LLVMOrcJIT LLVMMCJIT LLVMJITLink LLVMOrcTargetProcess LLVMOrcShared
        LLVMInterpreter LLVMExecutionEngine LLVMRuntimeDyld LLVMSymbolize LLVMDebugInfoPDB LLVMDebugInfoGSYM LLVMOption
        LLVMObjectYAML LLVMMCA LLVMMCDisassembler LLVMLTO LLVMCFGuard LLVMFrontendOpenACC LLVMExtensions Polly PollyISL
        LLVMPasses LLVMObjCARCOpts LLVMCoroutines LLVMipo LLVMInstrumentation LLVMVectorize LLVMLinker LLVMFrontendOpenMP
        LLVMDWARFLinker LLVMGlobalISel LLVMMIRParser LLVMAsmPrinter LLVMDebugInfoDWARF LLVMSelectionDAG LLVMCodeGen
        LLVMIRReader LLVMAsmParser LLVMInterfaceStub LLVMFileCheck LLVMFuzzMutate LLVMTarget LLVMScalarOpts
        LLVMInstCombine LLVMAggressiveInstCombine LLVMTransformUtils LLVMBitWriter LLVMAnalysis LLVMProfileData
        LLVMObject LLVMTextAPI LLVMMCParser LLVMMC LLVMDebugInfoCodeView LLVMDebugInfoMSF LLVMBitReader LLVMCore
        LLVMRemarks LLVMBitstreamReader LLVMBinaryFormat LLVMTableGen LLVMSupport LLVMDemangle
        LLVMWebAssemblyAsmParser LLVMWebAssemblyCodeGen LLVMWebAssemblyDesc LLVMWebAssemblyDisassembler
        LLVMWebAssemblyInfo LLVMWebAssemblyUtils
)
set(EOKAS_LIBRARY_FILES
        base ${LLVM_DEPS} ${LLVM_LIBS}
)

file(GLOB EOKAS_SOURCE_FILES
        "${EOKAS_TARGET_DIR}/src/*.cpp"
        "${EOKAS_TARGET_DIR}/src/app/*.cpp"
        "${EOKAS_TARGET_DIR}/src/ast/*.cpp"
        "${EOKAS_TARGET_DIR}/src/omis/*.cpp"
        "${EOKAS_TARGET_DIR}/src/omis/llvm/*.cpp"
)

message("EOKAS_HEADER_DIRS = ${EOKAS_HEADER_DIRS}")
message("EOKAS_LIBRARY_DIRS = ${EOKAS_LIBRARY_DIRS}")
message("EOKAS_HEADER_FILES = ${EOKAS_HEADER_FILES}")
message("EOKAS_SOURCE_FILES = ${EOKAS_SOURCE_FILES}")
message("EOKAS_LIBRARY_FILES = ${EOKAS_LIBRARY_FILES}")


add_executable(${EOKAS_TARGET_NAME} ${EOKAS_HEADER_FILES} ${EOKAS_SOURCE_FILES})
target_include_directories(${EOKAS_TARGET_NAME} PRIVATE ${EOKAS_HEADER_DIRS})
target_link_directories(${EOKAS_TARGET_NAME} PRIVATE ${EOKAS_LIBRARY_DIRS})
target_link_libraries(${EOKAS_TARGET_NAME} ${EOKAS_LIBRARY_FILES})
