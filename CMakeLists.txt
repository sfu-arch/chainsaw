project(ChainSaw-Sim)
cmake_minimum_required(VERSION 2.8)

set(PACKAGE_NAME ChainSaw)
set(PACKAGE_VERSION 1.01)
set(PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")
set(PACKAGE_BUGREPORT "amiralis@sfu.ca")

set(RUBY_PATH ${CMAKE_SOURCE_DIR}/gems-lib/ruby_clean/amd64-linux/generated/MESI_CMP_directory_m/)


set(CMAKE_CXX_FLAGS "-std=c++11 -O3 -fpermissive -Wno-write-strings -Wno-unused-label")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")

# Find and include boost 
set(Boost_USE_STATIC_LIBS OFF) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 
find_package(Boost 1.61.0 COMPONENTS graph system filesystem program_options iostreams) 

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS}) 
else()
    message( FATAL_ERROR "ChainSaw needs Boost 1.55 or higher, CMake will exit.")
endif()

add_custom_target(
    def
    COMMAND ln -s ${CMAKE_SOURCE_DIR}/def ${CMAKE_CURRENT_BINARY_DIR}/def
    COMMENT "=================== Generating soft links ..."
    )


# Adding ruby
# 3rdparty Libraries
include(CMakeLists-ExternalProjects.txt)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/gems-lib/ruby_clean/O3sim_ruby)
link_directories(${RUBY_PATH}/lib)

# Add subdirectories
add_subdirectory(src)

enable_testing()
add_test(
    NAME Test
    COMMAND ${CMAKE_BINARY_DIR}/bin/chainSaw -i ${CMAKE_SOURCE_DIR}/test/179_art_dfg.dot -a ${CMAKE_SOURCE_DIR}/test/axc-dump.out.gz -m ${CMAKE_SOURCE_DIR}/test/ooo-dump.out.gz
    )



set(CMAKE_INSTALL_RPATH "${RUBY_PATH}/lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
