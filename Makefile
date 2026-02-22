BUILD_DIR=build

CFLAGS=-std=c11 -O3 -Wextra -Wall -Werror
LDFLAGS=-I.

$(BUILD_DIR)/basic.o: test/basic.c zparsec.h $(BUILD_DIR)/.gitignore
	cc $(CFLAGS) $(LDFLAGS) $< -o $@

$(BUILD_DIR)/.gitignore:
	mkdir -p $(BUILD_DIR)
	echo "*" > $(BUILD_DIR)/.gitignore
