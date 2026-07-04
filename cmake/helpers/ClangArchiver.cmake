# Helios Engine Clang archiver setup
#
# Selects LLVM archiver tools when Clang is active so LTO archives are produced
# with compatible tooling.

include_guard(GLOBAL)

#[[
    helios_configure_clang_archiver()

    Configures CMAKE_AR and CMAKE_RANLIB for Clang toolchains. clang-cl prefers
    llvm-lib or MSVC lib.exe; Unix-like Clang prefers llvm-ar and llvm-ranlib.

    Example:
        include(ClangArchiver)
        helios_configure_clang_archiver()
]]
function(helios_configure_clang_archiver)
  if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    return()
  endif()

  if(MSVC OR CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    find_program(LLVM_LIB_EXECUTABLE
        NAMES llvm-lib llvm-lib.exe
        HINTS
            "$ENV{VCINSTALLDIR}/Tools/Llvm/x64/bin"
            "$ENV{VCINSTALLDIR}/Tools/Llvm/bin"
            "C:/Program Files/LLVM/bin"
            "C:/Program Files (x86)/LLVM/bin"
    )
    find_program(MSVC_LIB_EXECUTABLE NAMES lib lib.exe)

    if(LLVM_LIB_EXECUTABLE)
      set(CMAKE_AR "${LLVM_LIB_EXECUTABLE}" CACHE FILEPATH "Archiver" FORCE)
    elseif(MSVC_LIB_EXECUTABLE)
      set(CMAKE_AR "${MSVC_LIB_EXECUTABLE}" CACHE FILEPATH "Archiver" FORCE)
    endif()
    set(CMAKE_RANLIB "" CACHE FILEPATH "Ranlib" FORCE)
  else()
    find_program(LLVM_AR_EXECUTABLE llvm-ar)
    find_program(LLVM_RANLIB_EXECUTABLE llvm-ranlib)
    if(LLVM_AR_EXECUTABLE)
      set(CMAKE_AR "${LLVM_AR_EXECUTABLE}" CACHE FILEPATH "Archiver" FORCE)
    endif()
    if(LLVM_RANLIB_EXECUTABLE)
      set(CMAKE_RANLIB "${LLVM_RANLIB_EXECUTABLE}" CACHE FILEPATH "Ranlib" FORCE)
    endif()
  endif()
endfunction()
