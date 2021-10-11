EXAMPLE_MOD_DIR := $(USERMOD_DIR)

# Add all C files to SRC_USERMOD.
SRC_USERMOD += $(EXAMPLE_MOD_DIR)/usqlite_module.c
SRC_USERMOD += $(EXAMPLE_MOD_DIR)/usqlite_connection.c
SRC_USERMOD += $(EXAMPLE_MOD_DIR)/usqlite_cursor.c
SRC_USERMOD += $(EXAMPLE_MOD_DIR)/usqlite_row.c
SRC_USERMOD += $(EXAMPLE_MOD_DIR)/usqlite_file.c
SRC_USERMOD += $(EXAMPLE_MOD_DIR)/usqlite_vfs.c
SRC_USERMOD += $(EXAMPLE_MOD_DIR)/usqlite_utils.c
SRC_USERMOD += $(EXAMPLE_MOD_DIR)/usqlite.c

# We can add our module folder to include paths if needed
# This is not actually needed in this example.
CFLAGS_USERMOD += -I$(EXAMPLE_MOD_DIR)
SQLITE_MOD_DIR := $(USERMOD_DIR)
