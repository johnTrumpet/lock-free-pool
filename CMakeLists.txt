cmake_minimum_required(VERSION 3.8)

project(lfPoolTest)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -Wall -pedantic -g3 -fsanitize=thread")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O3 -DNDEBUG") 

set(TESTBIN_POOL testLFPool)
set(TESTFILES_POOL      test/src/mainPool.cpp  
                        test/src/lockFreePoolTest.cpp)

include_directories(include
                    test/include
                    $ENV{GTEST_DIR}/include)

link_directories($ENV{GTEST_DIR}/lib)

add_executable(${TESTBIN_POOL} ${TESTFILES_POOL})

add_test(NAME ${TESTBIN_POOL} COMMAND ${TESTBIN_POOL})

target_link_libraries(${TESTBIN_POOL} gtest pthread)

set_target_properties(${TESTBIN_POOL} PROPERTIES 
                                        CXX_STANDARD 17
                                        CXX_STANDARD_REQUIRED YES 
                                        CXX_EXTENSIONS NO)