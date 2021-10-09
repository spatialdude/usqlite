# Create an INTERFACE library for our C module.
add_library(usermod_usqlite INTERFACE)

# Add our source files to the lib
target_sources(usermod_sqlite3 INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/usqlite_module.c
    ${CMAKE_CURRENT_LIST_DIR}/usqlite_connection.c
    ${CMAKE_CURRENT_LIST_DIR}/usqlite_cursor.c
    ${CMAKE_CURRENT_LIST_DIR}/usqlite_file.c
    ${CMAKE_CURRENT_LIST_DIR}/usqlite_vfs.c
    ${CMAKE_CURRENT_LIST_DIR}/usqlite_utils.c
    ${CMAKE_CURRENT_LIST_DIR}/sqlite3.c
)

# Add the current directory as an include directory.
target_include_directories(usermod_usqlite INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

# Link our INTERFACE library to the usermod target.
target_link_libraries(usermod INTERFACE usermod_usqlite)
