# ---------------------------------------------------------------------------
# MODERNDBS
# ---------------------------------------------------------------------------

if (APPLE)
    list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt/llvm@13/lib/cmake")
endif (APPLE)

find_package(LLVM 16 REQUIRED CONFIG)
message(STATUS "LLVM_INCLUDE_DIRS = ${LLVM_INCLUDE_DIRS}")
message(STATUS "LLVM_INSTALL_PREFIX = ${LLVM_INSTALL_PREFIX}")

add_definitions(${LLVM_DEFINITIONS})
set(LLVM_LIBS LLVM)
