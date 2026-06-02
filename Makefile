BUILD_DIR=build
BENCH_DATA_DIR=bench/data

CFLAGS=-std=c11 -O3 -Wextra -Wall -Werror
LDFLAGS=-I.

test: $(BUILD_DIR)/basic.o  $(BUILD_DIR)/linkage.o

bench: $(BUILD_DIR)/attoparsec_csv $(BUILD_DIR)/csv_demo.o $(BENCH_DATA_DIR)/customers-1000000.csv

$(BUILD_DIR)/basic.o: test/basic.c cparsec.h $(BUILD_DIR)/.gitignore
	cc $(CFLAGS) $(LDFLAGS) $< -o $@

$(BUILD_DIR)/attoparsec_csv: bench/haskell/ParseCSV.hs $(BUILD_DIR)/.gitignore
	ghc -fforce-recomp -O2 -Wall -outputdir $(BUILD_DIR) $< -o $@

$(BUILD_DIR)/linkage.o: test/linkage/linkage.c test/linkage/a.c cparsec.h test/linkage/shared.h
	cc $(CFLAGS) $(LDFLAGS) test/linkage/linkage.c test/linkage/a.c -o $@

$(BUILD_DIR)/csv_demo.o: bench/c/csv_demo.c bench/c/CParseCSV.c bench/c/csv.h bench/c/utils.h cparsec.h $(BUILD_DIR)/.gitignore
	cc $(CFLAGS) $(LDFLAGS) bench/c/csv_demo.c bench/c/CParseCSV.c -o $@

$(BENCH_DATA_DIR)/customers-1000000.csv: $(BENCH_DATA_DIR)/customers-1000000.7z
	7z e -y $< -o$(BENCH_DATA_DIR) $(notdir $@)
# needed to not decompress everytime, 7z preserves archive original timestamp and that trips up make
	touch $@

$(BUILD_DIR)/.gitignore:
	mkdir -p $(BUILD_DIR)
	echo "*" > $(BUILD_DIR)/.gitignore
