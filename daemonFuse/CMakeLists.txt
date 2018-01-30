cmake_minimum_required(VERSION 3.0)
set(CMAKE_TOOLCHAIN_FILE $ENV{HOME}/arm-cross-comp-env/rpi-zero-w-toolchain.cmake)
project(SETR_TP2_CLIENT)

set(SOURCE_CLIENT setrfs.c fstools.c communications.c)

SET(GCC_WARNING_FLAGS "-Wall -Wextra -Wpedantic -Wduplicated-cond -Wlogical-op -Wnull-dereference -Wjump-misses-init -Wshadow")
SET(GCC_ADDRESS_SANITIZER_FLAGS "-fno-omit-frame-pointer") # -fsanitize=address")
SET(FUSE_FILE_OFFSET_FLAG "-D_FILE_OFFSET_BITS=64")
add_definitions("${FUSE_FILE_OFFSET_FLAG} ${GCC_WARNING_FLAGS} ${GCC_ADDRESS_SANITIZER_FLAGS}")

add_executable(tp2client ${SOURCE_CLIENT})
target_link_libraries(tp2client fuse)
