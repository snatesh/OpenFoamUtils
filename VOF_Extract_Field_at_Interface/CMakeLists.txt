cmake_minimum_required(VERSION 2.6)

project(ExtractAtInterface)

find_package(VTK REQUIRED)
include(${VTK_USE_FILE})

# find python headers and lib files
find_package(PythonLibs 2.7 REQUIRED)

# add thirdparty headers
include_directories(${CMAKE_SOURCE_DIR}/ThirdParty/matplotlib-cpp)
include_directories(${CMAKE_SOURCE_DIR}/ThirdParty/jsoncons/include)
# add python headers
include_directories(${PYTHON_INCLUDE_DIRS})

#if (ENABLE_MPI)
#	# find mpi
#	find_package(MPI REQUIRED)
#	# add mpi headers
#	include_directories(${MPI_INCLUDE_PATH})
#	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DENABLE_MPI")
#endif()

add_executable(ExtractAtInterface ExtractAtInterface.C)
target_link_libraries(ExtractAtInterface ${VTK_LIBRARIES} ${PYTHON_LIBRARIES})
if (ENABLE_MPI)
	target_link_libraries(ExtractAtInterface ${MPI_LIBRARIES})
endif()
