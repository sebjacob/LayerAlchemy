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

include_directories(
    ${NUKE_ROOT}/include
    ${CMAKE_SOURCE_DIR}/include/nuke/
    ${CMAKE_SOURCE_DIR}/src/nuke/
)

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
set(NUKE_PLUGIN_DESTINATION nuke/plugins/${NUKE_RELEASE})

set(NUKE_LAYERSET_LIBS
    LayerSet
    LayerSetKnob
)

# layer_alchemy libs
foreach(LIB ${NUKE_LAYERSET_LIBS})

    add_library(${LIB}
        STATIC
        ${CMAKE_SOURCE_DIR}/src/nuke/${LIB}.cpp
    )
    target_link_libraries(
        ${LIB}
        ${LAYERSET_LIBS}
        ${LIB_DDIMAGE}
    )
    set_target_properties(${LIB} PROPERTIES
        PREFIX ""
        PUBLIC_HEADER ${CMAKE_SOURCE_DIR}/include/nuke/${LIB}.h
    )

    install(
        TARGETS ${LIB}
        DESTINATION ${NUKE_PLUGIN_DESTINATION}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_PREFIX}/include/nuke
    )
endforeach()

# plugins

set(NUKE_PLUGINS 
    RemoveLayerSet
    MultiplyLayerSet
    GradeLayerSet
    FlattenLayerSet
    GradeBeauty
    GradeBeautyLayerSet
)

foreach(PLUGIN ${NUKE_PLUGINS})

    add_library(${PLUGIN} SHARED
        ${CMAKE_SOURCE_DIR}/src/nuke/${PLUGIN}.cpp
    )

    target_link_libraries(${PLUGIN}
        ${LAYERSET_LIBS}
        ${NUKE_LAYERSET_LIBS}
        ${LIB_DDIMAGE}
    )

    set_target_properties(${PLUGIN} PROPERTIES
        PREFIX ""
        COMPILE_FLAGS ""
        LINK_FLAGS ""
    )

    install(
        TARGETS ${PLUGIN}
        DESTINATION ${NUKE_PLUGIN_DESTINATION}
    )
endforeach()

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
