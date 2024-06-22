
set(LIBNAME modal_pipe)

# James' standard list of cmake flags
set(CMAKE_C_FLAGS "-std=gnu99 -Wall -Wextra -Wuninitialized \
	-Wunused-variable -Wdouble-promotion -Wmissing-prototypes \
	-Wmissing-declarations -Werror=undef -Wno-unused-function ${CMAKE_C_FLAGS}")


# for VOXL, install 64-bit libraries to lib64, 32-bit libs go in /usr/lib
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^aarch64")
	set(LIB_INSTALL_DIR /usr/lib64)
else()
	set(LIB_INSTALL_DIR /usr/lib)
endif()

if(PLATFORM MATCHES APQ8096)
	add_definitions(-DPLATFORM_APQ8096)
endif()


# Build lib from all source files
file(GLOB all_src_files ${CMAKE_SOURCE_DIR}/library/src/*.c)
add_library(${LIBNAME} SHARED ${all_src_files})

# for VOXL, install 64-bit libraries to lib64, 32-bit libs go in /usr/lib
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^aarch64")
	target_link_libraries(${LIBNAME} LINK_PUBLIC pthread)
else()
	target_link_libraries(${LIBNAME} LINK_PUBLIC pthread)
endif()


target_include_directories(${LIBNAME} PUBLIC include )
target_include_directories(${LIBNAME} PUBLIC .) # for mavlink folder

# make the include directory public for install
file(GLOB LIB_HEADERS ${CMAKE_SOURCE_DIR}/library/include/*.h)
set_target_properties(${LIBNAME} PROPERTIES PUBLIC_HEADER "${LIB_HEADERS}")

# make sure everything is installed where we want
# LIB_INSTALL_DIR comes from the parent cmake file
install(
	TARGETS			${LIBNAME}
	LIBRARY			DESTINATION ${LIB_INSTALL_DIR}
	RUNTIME			DESTINATION /usr/bin
	PUBLIC_HEADER	DESTINATION /usr/include
)
