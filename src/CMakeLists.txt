#CMakelist file for src directory

file(GLOB SOURCES *.cpp)                
add_executable(chainSaw ${SOURCES})
target_link_libraries(chainSaw ${Boost_LIBRARIES} z ruby)


add_custom_command(
    TARGET chainSaw
    POST_BUILD
    COMMAND ln -sf ${CMAKE_SOURCE_DIR}/def ${CMAKE_BINARY_DIR}/def
    COMMENT "=================== Generating soft links ====================="
    )

add_custom_command(
    TARGET chainSaw
    POST_BUILD
    COMMAND ln -sf ${CMAKE_SOURCE_DIR}/gems-lib/ruby_clean/DRAM ${CMAKE_BINARY_DIR}/DRAM
    )

add_custom_command(
    TARGET chainSaw
    POST_BUILD
    COMMAND ln -sf ${CMAKE_SOURCE_DIR}/gems-lib/ruby_clean/network ${CMAKE_BINARY_DIR}/network
    )

add_custom_command(
    TARGET chainSaw
    POST_BUILD
    COMMAND mkdir -p ${CMAKE_BINARY_DIR}/output
    )

#Create shared library
add_library(chainS SHARED ${SOURCES})

#install(TARGETS chainSaw 
install(TARGETS chainSaw chainS
    RUNTIME DESTINATION ${CMAKE_BINARY_DIR}/bin
    LIBRARY DESTINATION ${CMAKE_BINARY_DIR}/lib)

