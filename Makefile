CC := gcc
CFLAGS := -Wall -Wextra -std=c11 -pedantic -g -Og

# Directories
SRC_DIR := src
INC_DIR := includes
BUILD_DIR := build

TARGET := fdtool

# Source files
SRC_FILES := \
	$(SRC_DIR)/arena.c \
	$(SRC_DIR)/closure.c \
	$(SRC_DIR)/fds_parser.c \
	$(SRC_DIR)/keys.c \
	$(SRC_DIR)/min_cover.c \
	$(SRC_DIR)/normal_form.c \
	fdtool.c

# Object files in build/
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(filter $(SRC_DIR)/%.c,$(SRC_FILES))) \
	$(BUILD_DIR)/fdtool.o

.PHONY: target clean dirs

target: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -I$(INC_DIR) -o $@ $^

# Ensure build dir exists
dirs:
	mkdir -p $(BUILD_DIR)

# Compile main
$(BUILD_DIR)/fdtool.o: fdtool.c | dirs
	$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

# Compile src/*.c into build/*.o
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | dirs
	$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

clean:
	-rm -rf $(BUILD_DIR) $(TARGET)
