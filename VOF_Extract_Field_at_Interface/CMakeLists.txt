cmake_minimum_required(VERSION 2.6)

project(ExtractAtInterface)

set(VTK_DIR "/home/sachin/Software/VTK/build")
find_package(VTK REQUIRED)
include(${VTK_USE_FILE})

# find python headers and lib files
find_package(PythonLibs 2.7 REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/ThirdParty/matplotlib-cpp)

# add python headers
include_directories(${PYTHON_INCLUDE_DIRS})


add_executable(ExtractAtInterface ExtractAtInterface.C)
target_include_directories(ExtractAtInterface PRIVATE ${PYTHON_INCLUDE_DIRS})
target_link_libraries(ExtractAtInterface ${VTK_LIBRARIES} ${PYTHON_LIBRARIES})
