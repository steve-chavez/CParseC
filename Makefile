BUILD_DIR=build
BENCH_DATA_DIR=bench/data

CC ?= cc
CFLAGS=-std=c99 -Wextra -Wall -Werror
LDFLAGS=-I.

SRC = cparsec.h bench/c/*.[ch] test/*.[ch] test/linkage/*.[ch]

all: test bench

test: CFLAGS += -O0
test: $(BUILD_DIR)/basic $(BUILD_DIR)/unnamed $(BUILD_DIR)/simd $(BUILD_DIR)/linkage $(BUILD_DIR)/freestanding $(BUILD_DIR)/example

bench: CFLAGS += -O3
bench: $(BUILD_DIR)/attoparsec_csv $(BUILD_DIR)/csv_demo $(BUILD_DIR)/csv_simd_demo $(BUILD_DIR)/csv-rust-demo $(BENCH_DATA_DIR)/customers-1000000.csv

$(BUILD_DIR)/example: examples/example.c cparsec.h $(BUILD_DIR)/.gitignore
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@

$(BUILD_DIR)/basic: test/basic.c test/basic.h test/hosted.h test/assertions.h cparsec.h $(BUILD_DIR)/.gitignore
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@

$(BUILD_DIR)/unnamed: test/unnamed.c test/hosted.h test/assertions.h cparsec.h $(BUILD_DIR)/.gitignore
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@

$(BUILD_DIR)/simd: test/simd.c test/hosted.h test/assertions.h cparsec.h $(BUILD_DIR)/.gitignore
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@

$(BUILD_DIR)/attoparsec_csv: bench/haskell/ParseCSV.hs $(BUILD_DIR)/.gitignore
	ghc -fforce-recomp -O2 -Wall -outputdir $(BUILD_DIR) $< -o $@

$(BUILD_DIR)/linkage: test/linkage/linkage.c test/linkage/a.c cparsec.h test/linkage/shared.h
	$(CC) $(CFLAGS) $(LDFLAGS) test/linkage/linkage.c test/linkage/a.c -o $@

$(BUILD_DIR)/freestanding: test/freestanding.c test/basic.h test/assertions.h cparsec.h $(BUILD_DIR)/.gitignore
	$(CC) $(CFLAGS) $(LDFLAGS) -ffreestanding -c $< -o $@

$(BUILD_DIR)/csv_demo: bench/c/csv_demo.c bench/c/CParseCSV.c bench/c/csv.h bench/c/utils.h cparsec.h $(BUILD_DIR)/.gitignore
	$(CC) $(CFLAGS) $(LDFLAGS) bench/c/csv_demo.c bench/c/CParseCSV.c -o $@

$(BUILD_DIR)/csv_simd_demo: bench/c/csv_demo.c bench/c/csv_simd.c bench/c/csv.h bench/c/utils.h cparsec.h $(BUILD_DIR)/.gitignore
	$(CC) $(CFLAGS) $(LDFLAGS) bench/c/csv_demo.c bench/c/csv_simd.c -o $@

$(BUILD_DIR)/csv-rust-demo: bench/rust/Cargo.toml bench/rust/Cargo.lock bench/rust/src/main.rs
	cargo build --release --manifest-path bench/rust/Cargo.toml
	cp bench/rust/target/release/csv-rust-demo $@

$(BENCH_DATA_DIR)/customers-1000000.csv: $(BENCH_DATA_DIR)/customers-1000000.7z
	7z e -y $< -o$(BENCH_DATA_DIR) $(notdir $@)
# needed to not decompress everytime, 7z preserves archive original timestamp and that trips up make
	touch $@

$(BUILD_DIR)/.gitignore:
	mkdir -p $(BUILD_DIR)
	echo "*" > $(BUILD_DIR)/.gitignore

.PHONY: style
style:
	clang-format -i $(SRC)

.PHONY: style-check
style-check:
	clang-format -i $(SRC)
	git diff-index --exit-code HEAD -- '*.c' '*.h'
