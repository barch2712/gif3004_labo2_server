cmake_minimum_required(VERSION 3.0)
set(CMAKE_TOOLCHAIN_FILE $ENV{HOME}/arm-cross-comp-env/rpi-zero-w-toolchain.cmake)
project(SETR_TP2_SERVEUR)

set(SOURCE_SERVEUR telechargeur.c requete.c actions.c servermain.c communications.c)

SET(GCC_WARNING_FLAGS "-Wall -Wextra -Wpedantic -Wduplicated-cond -Wlogical-op -Wnull-dereference -Wjump-misses-init -Wshadow")
SET(GCC_ADDRESS_SANITIZER_FLAGS "-fno-omit-frame-pointer") # -fsanitize=address")
add_definitions("${GCC_WARNING_FLAGS} ${GCC_ADDRESS_SANITIZER_FLAGS}")

add_executable(tp2serveur ${SOURCE_SERVEUR})
target_link_libraries(tp2serveur curl)
