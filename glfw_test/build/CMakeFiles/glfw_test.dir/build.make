# CMAKE generated file: DO NOT EDIT!
# Generated by "MinGW Makefiles" Generator, CMake Version 4.0

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

SHELL = cmd.exe

# The CMake executable.
CMAKE_COMMAND = "D:\Program Files\CMake\bin\cmake.exe"

# The command to remove a file.
RM = "D:\Program Files\CMake\bin\cmake.exe" -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = E:\github\NightRain2\glfw_test

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = E:\github\NightRain2\glfw_test\build

# Include any dependencies generated for this target.
include CMakeFiles/glfw_test.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/glfw_test.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/glfw_test.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/glfw_test.dir/flags.make

CMakeFiles/glfw_test.dir/codegen:
.PHONY : CMakeFiles/glfw_test.dir/codegen

CMakeFiles/glfw_test.dir/glfw_test.cpp.obj: CMakeFiles/glfw_test.dir/flags.make
CMakeFiles/glfw_test.dir/glfw_test.cpp.obj: CMakeFiles/glfw_test.dir/includes_CXX.rsp
CMakeFiles/glfw_test.dir/glfw_test.cpp.obj: E:/github/NightRain2/glfw_test/glfw_test.cpp
CMakeFiles/glfw_test.dir/glfw_test.cpp.obj: CMakeFiles/glfw_test.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=E:\github\NightRain2\glfw_test\build\CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/glfw_test.dir/glfw_test.cpp.obj"
	C:\PROGRA~2\mingw64\bin\C__~1.EXE $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/glfw_test.dir/glfw_test.cpp.obj -MF CMakeFiles\glfw_test.dir\glfw_test.cpp.obj.d -o CMakeFiles\glfw_test.dir\glfw_test.cpp.obj -c E:\github\NightRain2\glfw_test\glfw_test.cpp

CMakeFiles/glfw_test.dir/glfw_test.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/glfw_test.dir/glfw_test.cpp.i"
	C:\PROGRA~2\mingw64\bin\C__~1.EXE $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E E:\github\NightRain2\glfw_test\glfw_test.cpp > CMakeFiles\glfw_test.dir\glfw_test.cpp.i

CMakeFiles/glfw_test.dir/glfw_test.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/glfw_test.dir/glfw_test.cpp.s"
	C:\PROGRA~2\mingw64\bin\C__~1.EXE $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S E:\github\NightRain2\glfw_test\glfw_test.cpp -o CMakeFiles\glfw_test.dir\glfw_test.cpp.s

# Object files for target glfw_test
glfw_test_OBJECTS = \
"CMakeFiles/glfw_test.dir/glfw_test.cpp.obj"

# External object files for target glfw_test
glfw_test_EXTERNAL_OBJECTS =

glfw_test.exe: CMakeFiles/glfw_test.dir/glfw_test.cpp.obj
glfw_test.exe: CMakeFiles/glfw_test.dir/build.make
glfw_test.exe: E:/github/NightRain2/glfw_test/lib/glew32.lib
glfw_test.exe: E:/github/NightRain2/glfw_test/lib/libglfw3.a
glfw_test.exe: E:/github/NightRain2/glfw_test/lib/libSDL2.a
glfw_test.exe: E:/github/NightRain2/glfw_test/lib/libSDL2main.a
glfw_test.exe: E:/github/NightRain2/glfw_test/lib/libSDL2_mixer.a
glfw_test.exe: CMakeFiles/glfw_test.dir/linkLibs.rsp
glfw_test.exe: CMakeFiles/glfw_test.dir/objects1.rsp
glfw_test.exe: CMakeFiles/glfw_test.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --bold --progress-dir=E:\github\NightRain2\glfw_test\build\CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable glfw_test.exe"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles\glfw_test.dir\link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/glfw_test.dir/build: glfw_test.exe
.PHONY : CMakeFiles/glfw_test.dir/build

CMakeFiles/glfw_test.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles\glfw_test.dir\cmake_clean.cmake
.PHONY : CMakeFiles/glfw_test.dir/clean

CMakeFiles/glfw_test.dir/depend:
	$(CMAKE_COMMAND) -E cmake_depends "MinGW Makefiles" E:\github\NightRain2\glfw_test E:\github\NightRain2\glfw_test E:\github\NightRain2\glfw_test\build E:\github\NightRain2\glfw_test\build E:\github\NightRain2\glfw_test\build\CMakeFiles\glfw_test.dir\DependInfo.cmake "--color=$(COLOR)"
.PHONY : CMakeFiles/glfw_test.dir/depend

