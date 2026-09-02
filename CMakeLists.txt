# Set the minimum version of CMake that can be used
cmake_minimum_required(VERSION 3.20)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
set(CMAKE_C_COMPILER_FORCED TRUE)
set(CMAKE_CXX_COMPILER_FORCED TRUE)

# Set target definition variables required by the toolchain file
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/target.cmake)

# Set the toolchain file for cross-compilation
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/gcc-arm-none-eabi.cmake)

# Project definition
project(FC_BSP)

# Enable CMake support for ASM and C/CPP languages
enable_language(C ASM)

set(PROJECT_NAME FC_BSP)

# Create an executable object type
add_executable(${PROJECT_NAME}
  src/drivers/spi/spi.c
  src/drivers/i2c/i2c.h
  src/drivers/i2c/i2c.c)

# Include additional CMake files for flags, components, and files
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/flags.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/components.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/files.cmake)

# Add sources to executable/library
target_sources(${PROJECT_NAME} PRIVATE
  # User defined sources
)

# Add include directories
target_include_directories(${PROJECT_NAME} PRIVATE
  # User defined include directories
  # PX4-vari "surucu-basina-klasor" duzeni: her surucu kendi klasorunde
  # .h+.c bir arada, bu yuzden her klasor ayri ayri include yoluna eklenir.
  ${CMAKE_CURRENT_SOURCE_DIR}/src/device
  ${CMAKE_CURRENT_SOURCE_DIR}/src/drivers
  ${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/gpio
  ${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/rcc
  ${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/nvic
  ${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/dma
  ${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/spi
  ${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/i2c
)

# Compiler options
target_compile_options(${PROJECT_NAME} PRIVATE
  # User defined compiler options
)

# Add additional defined symbols
target_compile_definitions(${PROJECT_NAME} PRIVATE
  # Add additional defined symbols here
  STM32F407xx
)

# Linker options
target_link_options(${PROJECT_NAME} PRIVATE 
  # User defined linker options
)

# Add additional linker search directories
target_link_directories(${PROJECT_NAME} PRIVATE
  # Add additional linker search directories here
)

# Add additional linker libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
  # Add additional linker libraries here
)
