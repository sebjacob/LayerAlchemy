find_program(PYTHON "python")
find_program(MKDOCS_ABSPATH NAMES mkdocs)
include(GNUInstallDirs)

get_filename_component(DOCUMENTATION_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/documentation" ABSOLUTE)

add_custom_target(documentation
    COMMAND ${MKDOCS_ABSPATH} build -c -s -v -q --site-dir ${CMAKE_CURRENT_BINARY_DIR}/site
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating mkdocs documentation in ${CMAKE_CURRENT_SOURCE_DIR}"
)

add_custom_target(documentation_package
    COMMENT "creating documentation package for ${PROJECT_PACKAGE_NAME}"
    COMMAND ${CMAKE_COMMAND} -E tar "cfvz" "${PROJECT_NAME}-${LAYER_ALCHEMY_VERSION_STRING}_documentation.tgz" "${DOCUMENTATION_INSTALL_DIR}"
    DEPENDS ${CMAKE_INSTALL_PREFIX} # make sure the install directory exists
    DEPENDS documentation
)

install(
    DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/site/
    DESTINATION ${DOCUMENTATION_INSTALL_DIR}
)
