# cmake file for LayerAlchemy Nuke plugins

#find_package(GLEW REQUIRED) # This is needed, but not right now.

if(EXISTS ${NUKE_ROOT})
    message(STATUS "Found Nuke: ${NUKE_ROOT}")
else()
    message(FATAL_ERROR "Couldn't find Nuke root (${NUKE_ROOT}). Please set $NUKE_ROOT with -DNUKE_ROOT")
endif()

if(APPLE)
    get_filename_component(real_nuke_version ${NUKE_ROOT} NAME)
    get_bundle_main_executable(${NUKE_ROOT}/${real_nuke_version}.app nuke_main_executable)
    get_filename_component(NUKE_ROOT ${nuke_main_executable} DIRECTORY)
    set(CMAKE_MACOSX_RPATH 1)
endif()

find_library(LIB_DDIMAGE DDImage ${NUKE_ROOT})

# get the Nuke version info
file(READ ${NUKE_ROOT}/include/DDImage/ddImageVersionNumbers.h ver)
string(REGEX MATCH "#define kDDImageVersionMajorNum ([0-9]*)" _ ${ver})
set(NUKE_MAJOR ${CMAKE_MATCH_1})
string(REGEX MATCH "#define kDDImageVersionMinorNum ([0-9]*)" _ ${ver})
set(NUKE_MINOR ${CMAKE_MATCH_1})
string(REGEX MATCH "#define kDDImageVersionReleaseNum ([0-9]*)" _ ${ver})
set(NUKE_RELEASE_NUM ${CMAKE_MATCH_1})

set(NUKE_RELEASE ${NUKE_MAJOR}.${NUKE_MINOR} CACHE STRING "" FORCE)

include_directories(
    ${NUKE_ROOT}/include
    ${CMAKE_SOURCE_DIR}/include/nuke/
    ${CMAKE_SOURCE_DIR}/src/nuke/
)
set(NUKE_PLUGIN_DESTINATION nuke/plugins/${NUKE_RELEASE})
set(PROJECT_NUKE_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include/nuke)
set(PROJECT_NUKE_SRC_DIR ${CMAKE_SOURCE_DIR}/src/nuke)

# layer_alchemy libs
add_library(LayerSet STATIC ${PROJECT_NUKE_SRC_DIR}/LayerSet.cpp)
target_link_libraries(LayerSet ${LAYERSET_LIBS} ${LIB_DDIMAGE})
set_target_properties(LayerSet
    PROPERTIES PREFIX ""
    PUBLIC_HEADER ${PROJECT_NUKE_INCLUDE_DIR}/LayerSet.h)
list(APPEND NUKE_LAYERSET_LIBS LayerSet)

add_library(LayerSetKnob STATIC ${PROJECT_NUKE_SRC_DIR}/LayerSetKnob.cpp)
target_link_libraries(LayerSetKnob LayerSet ${LAYERSET_LIBS} ${LIB_DDIMAGE})
set_target_properties(LayerSetKnob
    PROPERTIES PREFIX ""
    PUBLIC_HEADER ${PROJECT_NUKE_INCLUDE_DIR}/LayerSetKnob.h)
list(APPEND NUKE_LAYERSET_LIBS LayerSetKnob)

# plugins

add_library(RemoveLayerSet SHARED ${PROJECT_NUKE_SRC_DIR}/RemoveLayerSet.cpp)
list(APPEND NUKE_PLUGINS RemoveLayerSet)

add_library(MultiplyLayerSet SHARED ${PROJECT_NUKE_SRC_DIR}/MultiplyLayerSet.cpp)
list(APPEND NUKE_PLUGINS MultiplyLayerSet)

add_library(GradeLayerSet SHARED ${PROJECT_NUKE_SRC_DIR}/GradeLayerSet.cpp)
list(APPEND NUKE_PLUGINS GradeLayerSet)

add_library(FlattenLayerSet SHARED ${PROJECT_NUKE_SRC_DIR}/FlattenLayerSet.cpp)
list(APPEND NUKE_PLUGINS FlattenLayerSet)

add_library(GradeBeauty SHARED ${PROJECT_NUKE_SRC_DIR}/GradeBeauty.cpp)
list(APPEND NUKE_PLUGINS GradeBeauty)

add_library(GradeBeautyLayerSet SHARED ${PROJECT_NUKE_SRC_DIR}/GradeBeautyLayerSet.cpp)
list(APPEND NUKE_PLUGINS GradeBeautyLayerSet)

add_library(GradeBeautyLayer SHARED ${PROJECT_NUKE_SRC_DIR}/GradeBeautyLayer.cpp)
list(APPEND NUKE_PLUGINS GradeBeautyLayer)

foreach(PLUGIN ${NUKE_PLUGINS})
    set_target_properties(${PLUGIN} PROPERTIES PREFIX "")
    target_link_libraries(${PLUGIN} ${LAYERSET_LIBS} ${NUKE_LAYERSET_LIBS} ${LIB_DDIMAGE})
endforeach()

install(
    TARGETS ${NUKE_PLUGINS}
    DESTINATION ${NUKE_PLUGIN_DESTINATION}
)

install(
    TARGETS ${NUKE_LAYERSET_LIBS}
    DESTINATION ${NUKE_PLUGIN_DESTINATION}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_PREFIX}/include/nuke
)

# # copy menu and icons and configs over to the build Dir
install(
    DIRECTORY ${CMAKE_SOURCE_DIR}/icons
    DESTINATION nuke
    FILES_MATCHING PATTERN "*.png"
)

install(
    DIRECTORY ${CMAKE_SOURCE_DIR}/python/nuke
    DESTINATION ${CMAKE_INSTALL_PREFIX}
    FILES_MATCHING PATTERN "*.py"
)
