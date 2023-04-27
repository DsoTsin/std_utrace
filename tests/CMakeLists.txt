add_executable(test_commandline test_commandline.cpp)
target_include_directories(test_commandline PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../inc)
target_link_libraries(test_commandline PRIVATE unreal_trace)

add_executable(test_trace test_trace.cpp)
target_include_directories(test_trace PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../inc)
target_link_libraries(test_trace PRIVATE unreal_trace)