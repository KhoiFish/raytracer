cmake_minimum_required (VERSION 2.6)
project (RayTracerConsoleUnix)

# Set where to output the binary to
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/BuildOutput)

# Build with C++17 standard and pthread
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++17 -pthread")

# Specify the sources to build
include_directories(Source ThirdParty)
file(GLOB CORE_SOURCE "Source/Core/*.cpp")
set(STB_SOURCE  "ThirdParty/StbImage/StbImageImpl.cpp")
set(SAMPLE_SOURCE "Source/SampleScenes.cpp")
set(MAIN_SOURCE "Source/ConsoleMain.cpp")

# Build target
add_executable(${PROJECT_NAME} ${CORE_SOURCE} ${STB_SOURCE} ${SAMPLE_SOURCE} ${MAIN_SOURCE})
target_link_libraries(${PROJECT_NAME})
