# LayerAlchemy CMake build file
# For build instructions, consult README.md

cmake_minimum_required(VERSION 2.8)
include(BundleUtilities)
include(ExternalProject)

project(LayerAlchemy)

# get the LayerAlchemy version info
set(LAYER_ALCHEMY_VERSION_HEADER_FILE ${CMAKE_SOURCE_DIR}/include/version.h)
file(READ ${LAYER_ALCHEMY_VERSION_HEADER_FILE} version)
string(REGEX MATCH "#define LAYER_ALCHEMY_VERSION_MAJOR ([0-9]*)" _ ${version})
set(LAYER_ALCHEMY_VERSION_MAJOR ${CMAKE_MATCH_1})
string(REGEX MATCH "#define LAYER_ALCHEMY_VERSION_MINOR ([0-9]*)" _ ${version})
set(LAYER_ALCHEMY_VERSION_MINOR ${CMAKE_MATCH_1})
string(REGEX MATCH "#define LAYER_ALCHEMY_VERSION_PATCH ([0-9]*)" _ ${version})
set(LAYER_ALCHEMY_VERSION_PATCH ${CMAKE_MATCH_1})
set(LAYER_ALCHEMY_VERSION_STRING
    ${LAYER_ALCHEMY_VERSION_MAJOR}.${LAYER_ALCHEMY_VERSION_MINOR}.${LAYER_ALCHEMY_VERSION_PATCH}
)
option(BUILD_APPS "Build binary applications" ON)
option(BUILD_DOCS "Build html documentation" ON)
option(BUILD_NUKE "Build the Nuke plugins" ON)
option(VERBOSE "Add more verbosity to cmake" OFF)

set(NUKE_ROOT "/opt/nuke/Nuke12.0v1" CACHE PATH "Path to Nuke install root")
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-msse -O3 -DYAML=LAYER_ALCHEMY_YAML") # avoid Nuke 12 OCIO yaml-cpp namespace clashing
set(CMAKE_POSITION_INDEPENDENT_CODE ON) # Tells CMake to add -fPic flag.

if(UNIX AND NOT CYGWIN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 ")
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS})

# Placeholders for later, Nuke plugins can need this.
#find_package(GLEW REQUIRED)
#find_package(OpenGL REQUIRED)

ExternalProject_Add(yaml_cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG yaml-cpp-0.6.2
    # YAML_CPP_BUILD_TESTS -> Don't build tests
    # CMAKE_POSITION_INDEPENDENT_CODE -> Tells CMake to add -fPic flag.
    # By default yaml-cpp uses fPic only when building in shared mode.
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/libs -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS} -DYAML_CPP_BUILD_TESTS=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    PREFIX "${CMAKE_BINARY_DIR}/third-party/yaml_cpp"
)

ExternalProject_Add(argparse
    # A simple C++ header only command line argument parser
    # Jesse Laning https://github.com/jamolnng
    GIT_REPOSITORY https://github.com/jamolnng/argparse.git
    GIT_TAG v0.1.0
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    CONFIGURE_COMMAND ""
    UPDATE_COMMAND "" # for cmake cache,
    PREFIX "${CMAKE_BINARY_DIR}/third-party/argparse"
)

link_directories(
    ${CMAKE_CURRENT_BINARY_DIR}/libs/lib
)

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}/libs/include
    ${CMAKE_BINARY_DIR}/third-party/argparse/src/argparse
)

add_library(LayerSetConfig STATIC ${CMAKE_SOURCE_DIR}/src/LayerSetConfig.cpp)
add_dependencies(LayerSetConfig yaml_cpp)
target_link_libraries(LayerSetConfig PRIVATE yaml-cpp)
set_target_properties(LayerSetConfig PROPERTIES PUBLIC_HEADER ${CMAKE_SOURCE_DIR}/include/LayerSetConfig.h)
list(APPEND LAYERSET_LIBS LayerSetConfig)

add_library(LayerSetCore STATIC ${CMAKE_SOURCE_DIR}/src/LayerSetCore.cpp)
add_dependencies(LayerSetCore LayerSetConfig)
set_target_properties(LayerSetCore PROPERTIES PUBLIC_HEADER ${CMAKE_SOURCE_DIR}/include/LayerSetCore.h)
list(APPEND LAYERSET_LIBS LayerSetCore)

install(
    TARGETS ${LAYERSET_LIBS}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_PREFIX}/include
)

if(BUILD_APPS)
    add_executable(LayerTester ${CMAKE_SOURCE_DIR}/src/LayerTester.cpp)
    target_link_libraries(LayerTester LayerSetCore LayerSetConfig)
    add_dependencies(LayerTester argparse)

    add_executable(ConfigTester ${CMAKE_SOURCE_DIR}/src/ConfigTester.cpp)
    target_link_libraries(ConfigTester LayerSetCore LayerSetConfig)
    install(
        TARGETS LayerTester ConfigTester
        RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
    )
endif()

file(GLOB YAML_CONFIGS "${CMAKE_SOURCE_DIR}/configs/*.y*ml")
file(GLOB TEXT_FILES "${CMAKE_SOURCE_DIR}/*.md")

install(
    FILES ${YAML_CONFIGS}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/configs
)
install(
    FILES ${TEXT_FILES}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/
)
install(
    FILES ${LAYER_ALCHEMY_VERSION_HEADER_FILE}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include
)

if(BUILD_NUKE)
    add_subdirectory(src/nuke)
endif()

if(BUILD_DOCS)
    add_subdirectory(documentation)
endif()

# Section for packaging the project

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(platformName "osx")
else()
    string(TOLOWER ${CMAKE_SYSTEM_NAME} platformName)
endif()

set(PROJECT_PACKAGE_NAME "${PROJECT_NAME}-${LAYER_ALCHEMY_VERSION_STRING}-${platformName}")

if(BUILD_NUKE)
    set(PROJECT_PACKAGE_NAME "${PROJECT_NAME}-${LAYER_ALCHEMY_VERSION_STRING}-nuke-${NUKE_RELEASE}-${platformName}")
endif()


if (WIN32)
    set(PROJECT_PACKAGE_COMMAND "cfv" "${PROJECT_PACKAGE_NAME}.zip" --format=zip "${CMAKE_INSTALL_PREFIX}")
else()
    set(PROJECT_PACKAGE_COMMAND  "cfvz" "${PROJECT_PACKAGE_NAME}.tgz" "${CMAKE_INSTALL_PREFIX}")
endif()

add_custom_target(package
    COMMENT "creating package for ${PROJECT_PACKAGE_NAME}"
    COMMAND ${CMAKE_COMMAND} -E tar ${PROJECT_PACKAGE_COMMAND}
    DEPENDS ${CMAKE_INSTALL_PREFIX} # make sure the install directory exists
    DEPENDS documentation
)