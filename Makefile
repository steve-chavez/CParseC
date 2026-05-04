BUILD_DIR=build

CFLAGS=-std=c11 -O3 -Wextra -Wall -Werror
LDFLAGS=-I.

all: $(BUILD_DIR)/basic.o $(BUILD_DIR)/attoparsec_csv.o

$(BUILD_DIR)/basic.o: test/basic.c cparsec.h $(BUILD_DIR)/.gitignore
	cc $(CFLAGS) $(LDFLAGS) $< -o $@

$(BUILD_DIR)/attoparsec_csv.o: attoparsec/ParseCSV.hs $(BUILD_DIR)/.gitignore
	ghc -O2 -Wall -outputdir $(BUILD_DIR) $< -o $@

$(BUILD_DIR)/.gitignore:
	mkdir -p $(BUILD_DIR)
	echo "*" > $(BUILD_DIR)/.gitignore

