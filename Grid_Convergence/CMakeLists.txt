cmake_minimum_required(VERSION 2.6)

project(GridConvergence)

find_package(VTK REQUIRED)
include(${VTK_USE_FILE})

# find python headers and lib files
find_package(PythonLibs REQUIRED)

# add thirdparty headers
include_directories(${CMAKE_SOURCE_DIR}/../ThirdParty/matplotlib-cpp)
include_directories(${CMAKE_SOURCE_DIR}/../ThirdParty/jsoncons/include)
include_directories(${CMAKE_SOURCE_DIR}/../ThirdParty/eigen-git-mirror)
# add python headers
include_directories(${PYTHON_INCLUDE_DIRS})
# add project headers
include_directories(${CMAKE_SOURCE_DIR}/../VOF_Extract_Field_at_Interface/include)
include_directories(${CMAKE_SOURCE_DIR}/include)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O2")
# build contour lib
add_library(ContourInterface SHARED ${CMAKE_SOURCE_DIR}/../VOF_Extract_Field_at_Interface/src/ContourInterface.C)
target_link_libraries(ContourInterface ${VTK_LIBRARIES}) 
add_library(SplineInterp SHARED ${CMAKE_SOURCE_DIR}/src/SplineInterp.C)
add_library(OrderOfAccuracy SHARED ${CMAKE_SOURCE_DIR}/src/OrderOfAccuracy.C)
target_link_libraries(OrderOfAccuracy SplineInterp ContourInterface)
add_executable(GridConvergence GridConvergence.C)
target_link_libraries(GridConvergence OrderOfAccuracy ${PYTHON_LIBRARIES})
