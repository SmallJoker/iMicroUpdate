cmake_minimum_required(VERSION 2.6)
project(iMicroUpdate)

set(SRC_FILES
	main.cpp
	args_parser.cpp
)

if (MSVC)

else ()
	# -pg -g -O0
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wabi")
endif ()

add_executable(${PROJECT_NAME} ${SRC_FILES})

target_link_libraries(
	${PROJECT_NAME}
)
