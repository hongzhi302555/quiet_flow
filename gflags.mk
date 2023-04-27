BUILD_DIR = cpp3rdlib/gflags

libgflags: cpp3rdlib/gflags/lib/libgflags.so

gflags_temp:
	mkdir -p gflags_temp
	# cp v2.2.2.zip gflags_temp/gtest.zip
	curl -sL https://github.com/gflags/gflags/archive/refs/tags/v2.2.2.zip > gflags_temp/gtest.zip
	unzip gflags_temp/gtest.zip -d gflags_temp/
	mkdir -p gflags_temp/gflags-2.2.2/build
	cd gflags_temp/gflags-2.2.2/build; cmake -DBUILD_SHARED_LIBS=ON -DBUILD_STATIC_LIBS=ON -DINSTALL_HEADERS=ON -DINSTALL_SHARED_LIBS=ON -DINSTALL_STATIC_LIBS=ON ..; make -j6
	mv gflags_temp/gflags-2.2.2/build/* $(BUILD_DIR)
	rm -rf gflags_temp

$(BUILD_DIR)/lib/libgflags.so:
	mkdir -p $(BUILD_DIR)
	if [ ! -f "$@" ]; then make gflags_temp; fi


