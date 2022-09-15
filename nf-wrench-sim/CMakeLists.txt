
cmake_minimum_required(VERSION 3.2)
message(STATUS "Cmake version ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}")

project(nf-wrench-sim)

add_definitions("-Wall -Wno-unused-variable -Wno-unused-private-field")

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMakeModules/")

# Find WRENCH, SimGrid, and Boost
include(FindSimGrid.cmake)
include(FindWRENCH.cmake)
find_package(Boost REQUIRED)

# include directories
include_directories(include/ /usr/local/include/ /opt/local/include/ ${WRENCH_INCLUDE_DIR} ${SimGrid_INCLUDE_DIR} ${Boost_INCLUDE_DIR})

# source files
# set(SOURCE_FILES
#         include/Controller.h
#         src/Controller.cpp
#         src/Simulator.cpp
#         )
set(SOURCE_FILES
        include/NextflowWorkflowExecutor.h
        include/JSONWorkflow.h
        src/NextflowWorkflowExecutor.cpp
        src/BareMetalChain.cpp
        src/JSONWorkflow.cpp )

# generating the executable
add_executable(nf-wrench-sim ${SOURCE_FILES})
target_link_libraries(nf-wrench-sim WRENCH::WRENCH)
target_link_libraries(nf-wrench-sim SimGrid::SimGrid)
# Specify libraries to link
if (ENABLE_BATSCHED)
    target_link_libraries(nf-wrench-sim
            ${WRENCH_LIBRARY}
            ${SimGrid_LIBRARY}
            ${Boost_LIBRARIES}
#            ${WRENCH_WFCOMMONS_WORKFLOW_PARSER_LIBRARY}
            -lzmq)
else()
    target_link_libraries(nf-wrench-sim
            ${WRENCH_LIBRARY}
            ${SimGrid_LIBRARY}
            ${Boost_LIBRARIES}
#            ${WRENCH_WFCOMMONS_WORKFLOW_PARSER_LIBRARY}
            )
endif()

install(TARGETS nf-wrench-sim DESTINATION bin)
