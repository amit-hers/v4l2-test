
cmake_minimum_required(VERSION 3.3)
project(v4l2_project)

include(library/CMakeLists.txt)

#find_library(MODAL_PIPE modal_pipe HINTS "./library/build")

file(GLOB headers *.hpp)
# set sources
file(GLOB sources
	capture.cpp
	encoder.cpp
	utils.cpp
	)


	
message(${CMAKE_SOURCE_DIR}/library/build)

include_directories("./library/include/")

add_executable (v4l2_project  main.cpp ${headers} ${sources})

add_executable (image-client  modal-image-client.cpp ${headers} ${sources})

add_executable (image-server  modal-image-server.cpp ${headers} ${sources})  

target_link_libraries (v4l2_project v4l2 pthread modal_pipe)

target_link_libraries (image-client v4l2 pthread modal_pipe)

target_link_libraries (image-server v4l2 pthread modal_pipe)