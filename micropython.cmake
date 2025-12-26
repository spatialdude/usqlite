add_library(usermod_usqlite INTERFACE)

target_sources(usermod_usqlite INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/usqlite_module.c
    ${CMAKE_CURRENT_LIST_DIR}/usqlite_connection.c
    ${CMAKE_CURRENT_LIST_DIR}/usqlite_cursor.c
    ${CMAKE_CURRENT_LIST_DIR}/usqlite_row.c
    ${CMAKE_CURRENT_LIST_DIR}/usqlite_file.c
    ${CMAKE_CURRENT_LIST_DIR}/usqlite_mem.c
    ${CMAKE_CURRENT_LIST_DIR}/usqlite_vfs.c
    ${CMAKE_CURRENT_LIST_DIR}/usqlite_utils.c
    ${CMAKE_CURRENT_LIST_DIR}/usqlite.c
)

target_include_directories(usermod_usqlite INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(usermod INTERFACE usermod_usqlite)
