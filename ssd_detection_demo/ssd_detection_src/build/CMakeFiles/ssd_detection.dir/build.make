# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.10

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /opt/software/cmake-3.10.3-Linux-x86_64/bin/cmake

# The command to remove a file.
RM = /opt/software/cmake-3.10.3-Linux-x86_64/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /opt/SJTU_Hogwarts/ssd_detection_demo/ssd_detection_src

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /opt/SJTU_Hogwarts/ssd_detection_demo/ssd_detection_src/build

# Include any dependencies generated for this target.
include CMakeFiles/ssd_detection.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/ssd_detection.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/ssd_detection.dir/flags.make

CMakeFiles/ssd_detection.dir/ssd_detection.cc.o: CMakeFiles/ssd_detection.dir/flags.make
CMakeFiles/ssd_detection.dir/ssd_detection.cc.o: ../ssd_detection.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/opt/SJTU_Hogwarts/ssd_detection_demo/ssd_detection_src/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/ssd_detection.dir/ssd_detection.cc.o"
	/opt/software/gcc-linaro-5.4.1-2017.05-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-g++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/ssd_detection.dir/ssd_detection.cc.o -c /opt/SJTU_Hogwarts/ssd_detection_demo/ssd_detection_src/ssd_detection.cc

CMakeFiles/ssd_detection.dir/ssd_detection.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/ssd_detection.dir/ssd_detection.cc.i"
	/opt/software/gcc-linaro-5.4.1-2017.05-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /opt/SJTU_Hogwarts/ssd_detection_demo/ssd_detection_src/ssd_detection.cc > CMakeFiles/ssd_detection.dir/ssd_detection.cc.i

CMakeFiles/ssd_detection.dir/ssd_detection.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/ssd_detection.dir/ssd_detection.cc.s"
	/opt/software/gcc-linaro-5.4.1-2017.05-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /opt/SJTU_Hogwarts/ssd_detection_demo/ssd_detection_src/ssd_detection.cc -o CMakeFiles/ssd_detection.dir/ssd_detection.cc.s

CMakeFiles/ssd_detection.dir/ssd_detection.cc.o.requires:

.PHONY : CMakeFiles/ssd_detection.dir/ssd_detection.cc.o.requires

CMakeFiles/ssd_detection.dir/ssd_detection.cc.o.provides: CMakeFiles/ssd_detection.dir/ssd_detection.cc.o.requires
	$(MAKE) -f CMakeFiles/ssd_detection.dir/build.make CMakeFiles/ssd_detection.dir/ssd_detection.cc.o.provides.build
.PHONY : CMakeFiles/ssd_detection.dir/ssd_detection.cc.o.provides

CMakeFiles/ssd_detection.dir/ssd_detection.cc.o.provides.build: CMakeFiles/ssd_detection.dir/ssd_detection.cc.o


# Object files for target ssd_detection
ssd_detection_OBJECTS = \
"CMakeFiles/ssd_detection.dir/ssd_detection.cc.o"

# External object files for target ssd_detection
ssd_detection_EXTERNAL_OBJECTS =

ssd_detection: CMakeFiles/ssd_detection.dir/ssd_detection.cc.o
ssd_detection: CMakeFiles/ssd_detection.dir/build.make
ssd_detection: /opt/SJTU_Hogwarts/ssd_detection_demo/ocv3.4.10/lib/libopencv_dnn.so.3.4.10
ssd_detection: /opt/SJTU_Hogwarts/ssd_detection_demo/ocv3.4.10/lib/libopencv_highgui.so.3.4.10
ssd_detection: /opt/SJTU_Hogwarts/ssd_detection_demo/ocv3.4.10/lib/libopencv_ml.so.3.4.10
ssd_detection: /opt/SJTU_Hogwarts/ssd_detection_demo/ocv3.4.10/lib/libopencv_objdetect.so.3.4.10
ssd_detection: /opt/SJTU_Hogwarts/ssd_detection_demo/ocv3.4.10/lib/libopencv_shape.so.3.4.10
ssd_detection: /opt/SJTU_Hogwarts/ssd_detection_demo/ocv3.4.10/lib/libopencv_stitching.so.3.4.10
ssd_detection: /opt/SJTU_Hogwarts/ssd_detection_demo/ocv3.4.10/lib/libopencv_superres.so.3.4.10
ssd_detection: /opt/SJTU_Hogwarts/ssd_detection_demo/ocv3.4.10/lib/libopencv_videostab.so.3.4.10
ssd_detection: /opt/SJTU_Hogwarts/ssd_detection_demo/ocv3.4.10/lib/libopencv_calib3d.so.3.4.10
ssd_detection: /opt/SJTU_Hogwarts/ssd_detection_demo/ocv3.4.10/lib/libopencv_features2d.so.3.4.10
ssd_detection: /opt/SJTU_Hogwarts/ssd_detection_demo/ocv3.4.10/lib/libopencv_flann.so.3.4.10
ssd_detection: /opt/SJTU_Hogwarts/ssd_detection_demo/ocv3.4.10/lib/libopencv_photo.so.3.4.10
ssd_detection: /opt/SJTU_Hogwarts/ssd_detection_demo/ocv3.4.10/lib/libopencv_video.so.3.4.10
ssd_detection: /opt/SJTU_Hogwarts/ssd_detection_demo/ocv3.4.10/lib/libopencv_videoio.so.3.4.10
ssd_detection: /opt/SJTU_Hogwarts/ssd_detection_demo/ocv3.4.10/lib/libopencv_imgcodecs.so.3.4.10
ssd_detection: /opt/SJTU_Hogwarts/ssd_detection_demo/ocv3.4.10/lib/libopencv_imgproc.so.3.4.10
ssd_detection: /opt/SJTU_Hogwarts/ssd_detection_demo/ocv3.4.10/lib/libopencv_core.so.3.4.10
ssd_detection: CMakeFiles/ssd_detection.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/opt/SJTU_Hogwarts/ssd_detection_demo/ssd_detection_src/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable ssd_detection"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/ssd_detection.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/ssd_detection.dir/build: ssd_detection

.PHONY : CMakeFiles/ssd_detection.dir/build

CMakeFiles/ssd_detection.dir/requires: CMakeFiles/ssd_detection.dir/ssd_detection.cc.o.requires

.PHONY : CMakeFiles/ssd_detection.dir/requires

CMakeFiles/ssd_detection.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/ssd_detection.dir/cmake_clean.cmake
.PHONY : CMakeFiles/ssd_detection.dir/clean

CMakeFiles/ssd_detection.dir/depend:
	cd /opt/SJTU_Hogwarts/ssd_detection_demo/ssd_detection_src/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /opt/SJTU_Hogwarts/ssd_detection_demo/ssd_detection_src /opt/SJTU_Hogwarts/ssd_detection_demo/ssd_detection_src /opt/SJTU_Hogwarts/ssd_detection_demo/ssd_detection_src/build /opt/SJTU_Hogwarts/ssd_detection_demo/ssd_detection_src/build /opt/SJTU_Hogwarts/ssd_detection_demo/ssd_detection_src/build/CMakeFiles/ssd_detection.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/ssd_detection.dir/depend

