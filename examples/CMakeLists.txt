cmake_minimum_required(VERSION 3.0)
project(examples)

link_libraries(pthread)

aux_source_directory(. SRCS)

add_executable(example ${SRCS})
target_link_libraries(example mttimer)

