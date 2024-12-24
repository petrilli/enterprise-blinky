cmake_minimum_required(VERSION 3.20.0)
set(CMAKE_C_STANDARD 23)

# Add our custom boards to the build environment. This has to be done before pulling in the Zephyr macros
list(APPEND BOARD_ROOT ../..)

macro(app_set_runner_args)
    board_runner_args(dfu-util "--dfuse-modifiers=force:mass-erase")
    board_runner_args(pyocd "--erase")
    board_runner_args(nrfjprog "--erase")
endmacro()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(blinky)

# Options source from https://best.openssf.org/Compiler-Hardening-Guides/Compiler-Options-Hardening-Guide-for-C-and-C++.html
if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-O2 -Wall -Wformat -Wformat=2 -Wconversion -Wimplicit-fallthrough)
    add_compile_options(-Werror=format-security)
    add_compile_options(-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3)
    add_compile_options(-D_GLIBCXX_ASSERTIONS)
    add_compile_options(-fstrict-flex-arrays=3)
    add_compile_options(-fstack-clash-protection -fstack-protector-strong)
    add_compile_options(-Wl,-z,nodlopen -Wl,-z,noexecstack)
    add_compile_options(-Wl,-z,relro -Wl,-z,now)
    add_compile_options(-Wl,--as-needed -Wl,--no-copy-dt-needed-entries)
endif()

target_sources(app PRIVATE src/main.c)
