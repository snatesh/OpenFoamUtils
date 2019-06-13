cmake_minimum_required(VERSION 2.6)

project(ExtractAtInterface)

set(VTK_DIR "/home/srn324/Software/VTK/lib64/cmake/vtk-8.90" CACHE PATH "VTK directory override" FORCE)
message("VTK_DIR: ${VTK_DIR}")
find_package(VTK REQUIRED NO_MODULE)

##if (VTK_FOUND)
#  message("VTK_USE_FILE: ${VTK_USE_FILE}")
#  include(${VTK_USE_FILE})
#endif()
# find python headers and lib files
#find_package(PythonLibs REQUIRED)
find_package (Python3 3.6 COMPONENTS Interpreter Development REQUIRED)

# add thirdparty headers
include_directories(${CMAKE_SOURCE_DIR}/../ThirdParty/matplotlib-cpp)
include_directories(${CMAKE_SOURCE_DIR}/../ThirdParty/jsoncons/include)
include_directories(${CMAKE_SOURCE_DIR}/include)
# add python headers
message("PYTHON_INCLUDE_DIRS: ${Python3_INCLUDE_DIRS}")
include_directories(${Python3_INCLUDE_DIRS})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O2")

add_library(ContourInterface SHARED src/ContourInterface.C)
target_link_libraries(ContourInterface ${VTK_LIBRARIES}) 

add_executable(ExtractAtInterface ExtractAtInterface.C)
target_link_libraries(ExtractAtInterface ContourInterface ${Python3_LIBRARIES})
