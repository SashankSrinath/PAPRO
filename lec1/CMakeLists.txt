cmake_minimum_required(VERSION 2.8.9)
project(par_openmp_pthread)
 

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
find_package(OpenMP REQUIRED)

#Bring the headers, such as Student.h into the project
include_directories(include)
 
#Can manually add the sources using the set command as follows:
#set(SOURCES src/mainapp.cpp src/Student.cpp)
 
file(GLOB SOURCES "*.c")
 
add_executable(testStudent ${SOURCES})

target_link_libraries(testStudent PRIVATE ${SDL2_LIBRARIES} Threads::Threads m OpenMP::OpenMP_CXX)
