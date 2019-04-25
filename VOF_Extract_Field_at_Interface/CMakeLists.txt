cmake_minimum_required(VERSION 2.6)

project(ExtractAtInterface)

find_package(VTK REQUIRED)
include(${VTK_USE_FILE})

# find python headers and lib files
find_package(PythonLibs REQUIRED)

# add thirdparty headers
include_directories(${CMAKE_SOURCE_DIR}/../ThirdParty/matplotlib-cpp)
include_directories(${CMAKE_SOURCE_DIR}/../ThirdParty/jsoncons/include)
include_directories(${CMAKE_SOURCE_DIR}/include)
# add python headers
include_directories(${PYTHON_INCLUDE_DIRS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O2")

add_library(ContourInterface SHARED src/ContourInterface.C)
target_link_libraries(ContourInterface ${VTK_LIBRARIES}) 

add_executable(ExtractAtInterface ExtractAtInterface.C)
target_link_libraries(ExtractAtInterface ContourInterface ${PYTHON_LIBRARIES})
