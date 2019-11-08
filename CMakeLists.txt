# LayerAlchemy CMake build file
# For build instructions, consult README.md

cmake_minimum_required(VERSION 2.8)
include(BundleUtilities)
include(ExternalProject)

set(PROJECT_NAME LayerAlchemy)
set(LAYER_ALCHEMY_VERSION_MAJOR 0)
set(LAYER_ALCHEMY_VERSION_MINOR 7)
set(LAYER_ALCHEMY_VERSION_PATCH 0)

option(BUILD_APPS "Build binary applications" ON)
option(BUILD_DOCS "Build html documentation" ON)
option(BUILD_NUKE "Build the Nuke plugins" ON)
option(VERBOSE "Add more verbosity to cmake" OFF)

set(NUKE_ROOT "/opt/nuke/Nuke12.0v1" CACHE PATH "Path to Nuke install root")
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-msse -O3")
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
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/libs -DYAML_CPP_BUILD_TESTS=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    BUILD_COMMAND "$(MAKE)"  # $(MAKE) (with parenthesis, not brackets) allows to inherits make -j multi-threads.
)

ExternalProject_Add(argparse
    # A simple C++ header only command line argument parser
    # Jesse Laning https://github.com/jamolnng
    GIT_REPOSITORY https://github.com/jamolnng/argparse.git
    GIT_TAG master
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    CONFIGURE_COMMAND ""
    UPDATE_COMMAND ""
)

link_directories(
    ${CMAKE_CURRENT_BINARY_DIR}/libs/lib
)

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}/libs/include
    ${CMAKE_CURRENT_BINARY_DIR}/argparse-prefix/src/argparse
)

set(LAYERSET_LIBS 
    LayerSetConfig
    LayerSetCore
)
# loop over LAYERSET_LIBS 
foreach(LAYERSET_LIB ${LAYERSET_LIBS})
    add_library(${LAYERSET_LIB}
      STATIC
      ${CMAKE_SOURCE_DIR}/src/${LAYERSET_LIB}.cpp
    )
    add_dependencies(${LAYERSET_LIB} yaml_cpp)
    target_link_libraries(${LAYERSET_LIB} yaml-cpp)

    install(
      TARGETS ${LAYERSET_LIB}
      ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
    )
endforeach()

# include files
install(
    DIRECTORY ${CMAKE_SOURCE_DIR}/include
    DESTINATION ${CMAKE_INSTALL_PREFIX}
    FILES_MATCHING PATTERN "*.h*"
    PATTERN ".DS_Store" EXCLUDE
    PATTERN "@eaDir" EXCLUDE
)

if(BUILD_APPS)
    add_executable(LayerTester ${CMAKE_SOURCE_DIR}/src/LayerTester.cpp)
    add_executable(ConfigTester ${CMAKE_SOURCE_DIR}/src/ConfigTester.cpp)
    add_dependencies(LayerTester argparse)
    target_link_libraries(LayerTester LayerSetCore LayerSetConfig)
    target_link_libraries(ConfigTester LayerSetCore LayerSetConfig)
    install(
        TARGETS LayerTester
        RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
    )
    install(
        TARGETS ConfigTester
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

if(BUILD_NUKE)
    add_subdirectory(src/nuke)
endif(BUILD_NUKE)

add_subdirectory(documentation)

# Section for packaging the project

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(platformName "osx")
else()
    string(TOLOWER ${CMAKE_SYSTEM_NAME} platformName)
endif()

set(PROJECT_PACKAGE_NAME "${PROJECT_NAME}-${LAYER_ALCHEMY_VERSION_MAJOR}.${LAYER_ALCHEMY_VERSION_MINOR}.${LAYER_ALCHEMY_VERSION_PATCH}-${platformName}")

if(BUILD_NUKE)
    set(PROJECT_PACKAGE_NAME "${PROJECT_NAME}-${LAYER_ALCHEMY_VERSION_MAJOR}.${LAYER_ALCHEMY_VERSION_MINOR}.${LAYER_ALCHEMY_VERSION_PATCH}-nuke-${NUKE_RELEASE}-${platformName}")
endif()


if (WIN32)
    set(PROJECT_PACKAGE_COMMAND "cfv" "${PROJECT_PACKAGE_NAME}.zip" --format=zip "${CMAKE_INSTALL_PREFIX}")
else(WIN32)
    set(PROJECT_PACKAGE_COMMAND  "cfvz" "${PROJECT_PACKAGE_NAME}.tgz" "${CMAKE_INSTALL_PREFIX}")
endif(WIN32)

add_custom_target(package
    COMMENT "creating package for ${PROJECT_PACKAGE_NAME}"
    COMMAND ${CMAKE_COMMAND} -E tar ${PROJECT_PACKAGE_COMMAND}
    DEPENDS ${CMAKE_INSTALL_PREFIX} # make sure the install direcotry exists
)
list (APPEND EXCLUDE "CMakeLists.txt$" ".*" "@eaDir" "*.pyc") # default excludes
