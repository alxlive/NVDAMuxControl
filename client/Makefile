CXX := g++
CXXFLAGS := -O3 -Wall -fvisibility=hidden -std=c++14 
LDFLAGS := $(CXXFLAGS)

BUILD_DIR := build
DIST_DIR := dist
MKDIR_P := mkdir -p
OBJ_FILES := $(BUILD_DIR)/pywrap.o $(BUILD_DIR)/kernel_socket.o
CLIENT_APP := NVDAMuxControlClient.py
so_library := $(BUILD_DIR)/KernelSocketLib$(shell python3-config --extension-suffix)

# This is the target that gets built when one invokes `make` (the first target
# is the default). We require that target directories be created, then that the
# brightness app be built.
all: directories brightness_app

# Target "directories" itself depends on the two output directories, whose
# rules can be found below.
directories: $(BUILD_DIR) $(DIST_DIR)

$(BUILD_DIR):
	$(MKDIR_P) $(BUILD_DIR)

$(DIST_DIR):
	$(MKDIR_P) $(DIST_DIR)

# The brightness app depends on brightness.py and on the shared library.
brightness_app:$(CLIENT_APP) $(so_library)
	pyinstaller --onefile $(CLIENT_APP) --windowed --distpath $(DIST_DIR) --workpath $(BUILD_DIR) --noconfirm

# The shared library depends on the object files, so make will figure out how
# to build those first.
$(so_library):$(OBJ_FILES)
	$(CXX) $(LDFLAGS) -undefined dynamic_lookup $(python3 -m pybind11 --includes) -fPIC -shared -o $@ $^

# Syntax - targets ...: target-pattern: prereq-patterns ...
# In the case of the first target, foo.o, the target-pattern matches foo.o
# and sets the "stem" to be "foo".
# It then replaces the '%' in prereq-patterns with that stem to generate
# foo.cpp.
$(OBJ_FILES):build/%.o:%.cpp
	$(CXX) $(CXXFLAGS) -fPIC `python3 -m pybind11 --includes` -c $< -o $@

clean:
	rm -rf build/ dist/
