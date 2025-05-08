USE_LOCAL_SYMLINK = no
BUILD_DIR = build
BUILD_DIR_TESTS = build/tests
SRC_DIR = src
SRC_DIR_TESTS = test
UNITY_DIR = external/unity

ifeq ($(USE_LOCAL_SYMLINK),no)
EXTERNAL_INCLUDE = -Ilibebb/src -Iexternal/include
else
EXTERNAL_INCLUDE = -Ilnlibebb/src -Iexternal/include
endif

CC = gcc
PACKAGES = $(shell pkg-config --libs raylib) -lm
SANITIZE = -fsanitize=address
CFLAGS = $(PACKAGES) $(EXTERNAL_INCLUDE) -Wall -Wextra -Wshadow -pedantic -Wstrict-prototypes -march=native
CFLAGS_TEST = -DTEST -I$(UNITY_DIR) -I$(SRC_DIR) -I$(EXTERNAL_INCLUDE) -ggdb $(SANITIZE) -std=c23

CFLAGS_DEBUG = $(CFLAGS) -DDEBUG -ggdb
CFLAGS_ASAN = $(CFLAGS) -DDEBUG $(SANITIZE)
CFLAGS_RELEASE = $(CFLAGS) -DNDEBUG -Ofast

# Arguments to append to the program run with "make run"
ARGS = 

# Build program

ifeq ($(USE_LOCAL_SYMLINK),no)
SRC = $(wildcard $(SRC_DIR)/*.c) $(wildcard libebb/src/*.c)
else
SRC = $(wildcard $(SRC_DIR)/*.c) $(wildcard lnlibebb/src/*.c)
endif

debug: $(BUILD_DIR) $(BUILD_DIR)/debug
release: $(BUILD_DIR) $(BUILD_DIR)/release
install: release
	cp $(BUILD_DIR)/release /usr/bin/bricklayer
asan: $(BUILD_DIR) $(BUILD_DIR)/asan

run: $(BUILD_DIR) $(BUILD_DIR)/release
	@echo "Warning: no address sanitation enabled, consider running with 'make run_asan' when developing."
	$(BUILD_DIR)/release $(ARGS)

run_asan: $(BUILD_DIR) $(BUILD_DIR)/asan
	$(BUILD_DIR)/asan $(ARGS)

$(BUILD_DIR)/debug: $(SRC)
	@echo "Building debug build"
	$(CC) -o $@ $^ $(CFLAGS_DEBUG)

$(BUILD_DIR)/release: $(SRC)
	@echo "Building release build"
	$(CC) -o $@ $^ $(CFLAGS_RELEASE)

$(BUILD_DIR)/asan: $(SRC)
	@echo "Building address sanitation build"
	$(CC) -o $@ $^ $(CFLAGS_ASAN)


$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR_TESTS):
	mkdir -p $(BUILD_DIR_TESTS)


# Build and run tests

SRC_FOR_TESTS = $(filter-out $(SRC_DIR)/main.c, $(SRC)) $(wildcard $(UNITY_DIR)/*.c)
OBJS_TESTS = $(patsubst $(SRC_DIR_TESTS)/%.c, $(BUILD_DIR_TESTS)/%.o, $(wildcard $(SRC_DIR_TESTS)/test_*.c))

test: $(BUILD_DIR_TESTS) run_tests
	@echo

NOOP=
SPACE = $(NOOP) $(NOOP)

run_tests: $(OBJS_TESTS)
	@echo -e "\n\n--------------\n Test results\n--------------\n"
	@$(subst $(SPACE), && echo -e "\n" && ,$^)

$(OBJS_TESTS): $(BUILD_DIR_TESTS)/%.o: $(SRC_DIR_TESTS)/%.c $(SRC_FOR_TESTS)
	@echo -e "\nBuilding $@"
	$(CC) -o $@ $^ $(CFLAGS_TEST)

clean:
	rm -rf $(BUILD_DIR)

