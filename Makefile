BUILD_DIR=build

CFLAGS=-std=c11 -O3 -Wextra -Wall -Werror
LDFLAGS=-I.

test: $(BUILD_DIR)/basic.o  $(BUILD_DIR)/linkage.o

bench: $(BUILD_DIR)/attoparsec_csv

$(BUILD_DIR)/basic.o: test/basic.c cparsec.h $(BUILD_DIR)/.gitignore
	cc $(CFLAGS) $(LDFLAGS) $< -o $@

$(BUILD_DIR)/attoparsec_csv: bench/haskell/ParseCSV.hs $(BUILD_DIR)/.gitignore
	ghc -fforce-recomp -O2 -Wall -outputdir $(BUILD_DIR) $< -o $@

$(BUILD_DIR)/linkage.o: test/linkage/linkage.c test/linkage/a.c cparsec.h test/linkage/shared.h
	cc $(CFLAGS) $(LDFLAGS) test/linkage/linkage.c test/linkage/a.c -o $@

$(BUILD_DIR)/.gitignore:
	mkdir -p $(BUILD_DIR)
	echo "*" > $(BUILD_DIR)/.gitignore
