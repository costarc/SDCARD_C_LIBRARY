# Compiler settings
CC = gcc
CFLAGS = -Wall -O2 -g

# Directories
SRC_DIR = .
OBJ_DIR = obj
BIN_DIR = bin

# Source files and object files
SRC_FILES = $(SRC_DIR)/sdbrowser.c $(SRC_DIR)/diskio.c $(SRC_DIR)/ff.c $(SRC_DIR)/ffsystem.c $(SRC_DIR)/ffunicode.c $(SRC_DIR)/FILE_disk_initialize.c $(SRC_DIR)/FILE_ioctl.c $(SRC_DIR)/FILE_disk_read.c $(SRC_DIR)/FILE_disk_write.c

OBJ_FILES = $(SRC_FILES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)  # Correct path for object files

# Target binary
TARGET = $(BIN_DIR)/sdbrowser

# Make all
all: $(TARGET)

# Linking the object files into a binary
$(TARGET): $(OBJ_FILES)
	@mkdir -p $(BIN_DIR)
	$(CC) $(OBJ_FILES) -o $(TARGET)

# Compiling the source files into object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build artifacts
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

# Phony targets
.PHONY: all clean
