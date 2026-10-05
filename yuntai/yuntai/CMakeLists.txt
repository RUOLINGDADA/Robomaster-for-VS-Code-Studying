cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME yuntai)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    Core/Src/app/gimbal/gimbal_axis.c
    Core/Src/app/gimbal/gimbal_control.c
    Core/Src/algorithm/pid/pid.c
    Core/Src/algorithm/ramp/ramp.c
    Core/Src/algorithm/filter/low_pass_filter.c
    Core/Src/bsp/c610_m2006/c610_m2006.c
    Core/Src/bsp/gm6020/gm6020.c
    Core/Src/bsp/can/can_rx_dispatch.c
    Core/Src/bsp/dbus/dbus.c
    Core/Src/bsp/c610_m2006/test_c610_m2006_self_cycle.c
    Core/Src/bsp/gm6020/test_gm6020_calibration.c
    Core/Src/bsp/gm6020/test_gm6020_angle_loop.c
    Core/Src/task/task_feed_motor/task_feed_motor.c
    Core/Src/task/task_feed_motor/task_feed_motor_command.c
    Core/Src/task/task_feed_motor/task_feed_motor_control.c
    Core/Src/task/task_feed_motor/task_feed_motor_runtime.c
    Core/Src/task/task_yaw/task_yaw.c
    Core/Src/task/task_yaw/task_yaw_command.c
    Core/Src/task/task_yaw/task_yaw_runtime.c
    Core/Src/task/task_pitch/task_pitch.c
    Core/Src/task/task_pitch/task_pitch_command.c
    Core/Src/task/task_pitch/task_pitch_runtime.c
    Core/Src/task/task_dbus/task_dbus.c
    Core/Src/task/task_dbus/task_dbus_runtime.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    Core/Inc
    Core/Inc/bsp/c610_m2006
    Core/Inc/bsp/gm6020
    Core/Inc/algorithm
    Core/Inc/task
    Core/Inc/app
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)
