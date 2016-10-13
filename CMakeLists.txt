cmake_minimum_required(VERSION 2.8)
project(cosc)

file(GLOB SOURCES *.c)
file(GLOB HEADERS *.h)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

install(TARGETS ${PROJECT_NAME} DESTINATION lib)
install(FILES ${HEADERS} DESTINATION include/${PROJECT_NAME})
