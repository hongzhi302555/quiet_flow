BUILD_DIR = cpp3rdlib/gtest

libgtest: $(BUILD_DIR) $(BUILD_DIR)/libgtest.a

$(BUILD_DIR)/libgtest.a: $(BUILD_DIR)/gtest-all.o $(BUILD_DIR)/gtest_main.o 
	ar rcs $(BUILD_DIR)/libgtest.a $^
	rm -rf gtest_temp

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)/include

gtest_temp:
	mkdir -p gtest_temp
	curl -sL https://github.com/google/googletest/archive/refs/heads/v1.8.x.zip > gtest_temp/gtest.zip
	unzip gtest_temp/gtest.zip -d gtest_temp/
	cp -r gtest_temp/googletest-1.8.x/googletest/include/ $(BUILD_DIR)/
	cp -r gtest_temp/googletest-1.8.x/googletest/include/gtest/* $(BUILD_DIR)/include
	cp -r gtest_temp/googletest-1.8.x/googletest/src $(BUILD_DIR)/src

$(BUILD_DIR)/src/gtest%.cc:
	if [ ! -f "$@" ]; then make gtest_temp; fi

$(BUILD_DIR)/gtest%.o: $(BUILD_DIR)/src/gtest%.cc
	$(CXX) -I $(BUILD_DIR) -I $(BUILD_DIR)/include -c $< -o $@