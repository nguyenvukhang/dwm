BUILD_DIR := target

current: build

configure:
	cmake -S . -B $(BUILD_DIR)

build:
	cmake --build $(BUILD_DIR)

install: build
	cmake --install $(BUILD_DIR)
