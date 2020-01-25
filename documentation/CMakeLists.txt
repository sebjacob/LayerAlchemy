find_program(PYTHON "python")
find_program(MKDOCS_ABSPATH NAMES mkdocs)
include(GNUInstallDirs)

get_filename_component(DOCUMENTATION_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/documentation" ABSOLUTE)

add_custom_target(documentation ALL
    COMMAND ${MKDOCS_ABSPATH} build -c -s -v -q --site-dir ${DOCUMENTATION_BUILD_DIR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating mkdocs documentation in ${CMAKE_CURRENT_SOURCE_DIR}"
)

add_custom_target(documentation_package
    DEPENDS documentation
    COMMENT "creating documentation package for ${PROJECT_PACKAGE_NAME}"
    COMMAND ${CMAKE_COMMAND} -E tar "cfvz" "${PROJECT_NAME}-${LAYER_ALCHEMY_VERSION_STRING}-documentation.tar.gz" "${CMAKE_CURRENT_BINARY_DIR}/documentation"
    COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_NAME}-${LAYER_ALCHEMY_VERSION_STRING}-documentation.tar.gz" "../"
)

install(
    DIRECTORY ${DOCUMENTATION_BUILD_DIR}
    DESTINATION ${CMAKE_INSTALL_PREFIX}
)
