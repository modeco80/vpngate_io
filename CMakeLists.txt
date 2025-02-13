cmake_minimum_required(VERSION 3.25)

project(vpngate_io)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_library(vpngate_io
    src/lib/easycrypt.cpp
    src/lib/dat_file.cpp
)
target_include_directories(vpngate_io PUBLIC ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(vpngate_io PUBLIC
# TODO do this properly.
    z
    crypto
)


add_executable(test src/test.cpp)
target_link_libraries(test 
    vpngate_io
)