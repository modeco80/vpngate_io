cmake_minimum_required(VERSION 3.25)

project(vpngate_io)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ZLIB REQUIRED)
find_package(OpenSSL REQUIRED)

option(VGIO_BUILD_UTILITIES "Build utilities" ON)
option(VGIO_BUILD_TESTUTILS "Build test utilities" OFF)
option(VGIO_BUILD_CAPI "Build the C API Bindings" ON)

add_library(vpngate_io
    src/lib/easycrypt.cpp
    src/lib/dat_file.cpp
    src/lib/pack_reader.cpp
    src/lib/simple.cpp
)

if(VGIO_BUILD_CAPI)
    message(STATUS "Building capi bindings")
    target_sources(vpngate_io PRIVATE
        # C API
        src/lib/capi/error.cpp
        src/lib/capi/simple.cpp
        src/lib/capi/pack_reader.cpp
    )
endif()

target_include_directories(vpngate_io PUBLIC ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(vpngate_io PUBLIC
    ZLIB::ZLIB
    OpenSSL::Crypto
)

if(VGIO_BUILD_UTILITIES)
    # Boost is only required for a utility build
    find_package(Boost REQUIRED COMPONENTS json)

    add_executable(vpngate_dat2json src/dat2json.cpp)
    target_link_libraries(vpngate_dat2json 
        vpngate_io
        Boost::json
    )

    add_executable(vpngate_datid src/datid.cpp)
    target_link_libraries(vpngate_datid 
        vpngate_io
    )

endif()

if(VGIO_BUILD_TESTUTILS)
    add_executable(capi_test src/capi_test.c)
    target_link_libraries(capi_test 
        vpngate_io
    )
endif()