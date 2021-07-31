get_filename_component(EXEC_NAME "${CMAKE_CURRENT_SOURCE_DIR}" NAME)
string(REPLACE " " "_" EXEC_NAME "${EXEC_NAME}")

include(GenerateExportHeader)

set(Header_Files "bithacks.h" "cripple_log.h" "stun.h")
source_group("Header Files" FILES "${Header_Files}")

set(Source_Files "cripple_log.c" "server.c")
source_group("Source Files" FILES "${Source_Files}")

add_library("${EXEC_NAME}" "${Header_Files}" "${Source_Files}")

target_include_directories(
        "${EXEC_NAME}"
        INTERFACE
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
        "$<INSTALL_INTERFACE:include>"
)

set(CMAKE_MODULE_PATH cmake)
find_package(LIBNET)
find_package(Libev)
target_include_directories("${EXEC_NAME}" PRIVATE "${LIBEV_INCLUDE_DIR}" "${LIBNET_INCLUDE_DIR}")
target_link_libraries("${EXEC_NAME}" PRIVATE "${LIBEV_LIBRARIES}" "${LIBNET_LIBRARY}")

target_link_libraries("${EXEC_NAME}" PUBLIC "${PROJECT_LOWER_NAME}_compiler_flags")

target_link_libraries("${EXEC_NAME}" PUBLIC "utils")

set_target_properties(
        "${EXEC_NAME}"
        PROPERTIES
        LINKER_LANGUAGE
        C
)

set(_export_file "${CMAKE_CURRENT_SOURCE_DIR}/${EXEC_NAME}_export.h")
generate_export_header("${EXEC_NAME}" EXPORT_FILE_NAME "${_export_file}")

# setup the version numbering
set_property(TARGET "${EXEC_NAME}" PROPERTY VERSION "1.0.0")
set_property(TARGET "${EXEC_NAME}" PROPERTY SOVERSION "1")

# install rules
set(installable_libs "${EXEC_NAME}" "${PROJECT_LOWER_NAME}_compiler_flags")
if (TARGET "${DEPENDANT_LIBRARY}")
  list(APPEND installable_libs "${DEPENDANT_LIBRARY}")
endif ()
install(TARGETS ${installable_libs}
        DESTINATION "bin"
        EXPORT "${EXEC_NAME}Targets")
install(FILES "${_export_file}" "${Header_Files}" DESTINATION "include")
