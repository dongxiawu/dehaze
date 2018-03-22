#cmake version
cmake_minimum_required(VERSION 3.8)

#project name
project(dehaze)

#add c++11 feature
set(CMAKE_CXX_STANDARD 11)

#
include_directories(include)

#
aux_source_directory(src SOURCE_LIST)

# Find OpenCV, you may need to set OpenCV_DIR variable
# to the absolute path to the directory containing OpenCVConfig.cmake file
# via the command line or GUI
find_package(OpenCV REQUIRED)

#
#add_executable(${PROJECT_NAME} ${SOURCE_FILES})
add_executable(${PROJECT_NAME} ${SOURCE_LIST})

# Link your application with OpenCV libraries
target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS})