find_program(PYTHON "python")
find_program(MKDOCS_ABSPATH NAMES mkdocs)
include(GNUInstallDirs)

add_custom_target(documentation
    COMMAND ${MKDOCS_ABSPATH} build -c -s -v -q --site-dir ${CMAKE_CURRENT_BINARY_DIR}/site
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating mkdocs documentation in ${CMAKE_CURRENT_SOURCE_DIR}"
)

install(
    DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/site/
    DESTINATION ${CMAKE_INSTALL_PREFIX}/documentation
)
