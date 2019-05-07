cmake_minimum_required(VERSION 3.0)
project(mttimer)

add_library(mttimer SHARED mt_timer.c)
set_target_properties(mttimer PROPERTIES VERSION 0.4 SOVERSION 1)

install (TARGETS mttimer DESTINATION lib)
install (FILES mt_timer.h DESTINATION include)
install (FILES uthash.h DESTINATION include)
