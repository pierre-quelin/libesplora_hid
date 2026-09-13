cmake_minimum_required(VERSION 3.25)

project(libesplora_hid VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(MAIN_SRCS src/esplora/hid/esplora_hid.cpp)
set(MAIN_HDRS
    src/esplora/hid/esplora_hid.hpp
    src/esplora/hid/esplora_hid_types.hpp
)

include(tools/LibDist.cmake)
set(_DIST_ROOT "${ESPLORA_HID_DIST_ROOT}")

add_library(${PROJECT_NAME} STATIC ${MAIN_SRCS} ${MAIN_HDRS})
set_source_files_properties(${MAIN_HDRS} PROPERTIES HEADER_FILE_ONLY TRUE)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "Source Files" FILES ${MAIN_SRCS})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "Header Files" FILES ${MAIN_HDRS})

target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include>
)

if ($ENV{BUILD_TARGET} MATCHES "msvc[0-9]+")
    add_compile_definitions(NOMINMAX NOGDI)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
    if(NOT (MSVC_VERSION LESS 1910))
        target_compile_options(${PROJECT_NAME} PRIVATE
            $<$<CXX_COMPILER_ID:MSVC>:/std:c++17 /permissive- /Zc:__cplusplus /EHsc /W4>
        )
    endif()
endif()

include(tools/Fetchlibusb.cmake)

if($ENV{BUILD_TARGET} MATCHES "linux-x86.*")
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBUSB REQUIRED libusb-1.0)
    target_link_libraries(${PROJECT_NAME} PUBLIC ${LIBUSB_LIBRARIES})
    target_include_directories(${PROJECT_NAME} PRIVATE ${LIBUSB_INCLUDE_DIRS})
elseif($ENV{BUILD_TARGET} STREQUAL "mingw64")
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(LIBUSB libusb-1.0)
    endif()
    if(LIBUSB_FOUND)
        target_link_libraries(${PROJECT_NAME} PUBLIC ${LIBUSB_LIBRARIES})
        target_include_directories(${PROJECT_NAME} PRIVATE ${LIBUSB_INCLUDE_DIRS})
    else()
        target_link_libraries(${PROJECT_NAME} PUBLIC usb-1.0)
    endif()
elseif($ENV{BUILD_TARGET} MATCHES "msvc[0-9]+")
    target_link_libraries(${PROJECT_NAME} PUBLIC libusb::libusb)
endif()

install(TARGETS ${PROJECT_NAME}
        ARCHIVE DESTINATION ${_DIST_ROOT}/lib
        LIBRARY DESTINATION ${_DIST_ROOT}/lib
        RUNTIME DESTINATION ${_DIST_ROOT})

install(FILES
        src/esplora/hid/esplora_hid.hpp
        src/esplora/hid/esplora_hid_types.hpp
        DESTINATION ${_DIST_ROOT}/include/esplora/hid)

if ($ENV{BUILD_TARGET} MATCHES "msvc[0-9]+" OR MSVC)
    if(TARGET libusb::libusb)
        get_target_property(_LIBUSB_RUNTIME_DLL libusb::libusb IMPORTED_LOCATION_RELEASE)
        if(NOT _LIBUSB_RUNTIME_DLL OR _LIBUSB_RUNTIME_DLL MATCHES "NOTFOUND")
            get_target_property(_LIBUSB_RUNTIME_DLL libusb::libusb IMPORTED_LOCATION)
        endif()
        if(_LIBUSB_RUNTIME_DLL AND NOT _LIBUSB_RUNTIME_DLL MATCHES "NOTFOUND" AND EXISTS "${_LIBUSB_RUNTIME_DLL}")
            install(FILES "${_LIBUSB_RUNTIME_DLL}" DESTINATION ${_DIST_ROOT} COMPONENT LIBRARIES)
        endif()
    endif()
endif()
