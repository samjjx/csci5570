# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.12

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
CMAKE_COMMAND = /Applications/CLion.app/Contents/bin/cmake/mac/bin/cmake

# The command to remove a file.
RM = /Applications/CLion.app/Contents/bin/cmake/mac/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/faraway/ps-project/feature_csci5570

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/faraway/ps-project/feature_csci5570/cmake-build-debug

# Utility rule file for glog.

# Include the progress variables for this target.
include CMakeFiles/glog.dir/progress.make

CMakeFiles/glog: CMakeFiles/glog-complete


CMakeFiles/glog-complete: third_party/src/glog-stamp/glog-install
CMakeFiles/glog-complete: third_party/src/glog-stamp/glog-mkdir
CMakeFiles/glog-complete: third_party/src/glog-stamp/glog-download
CMakeFiles/glog-complete: third_party/src/glog-stamp/glog-update
CMakeFiles/glog-complete: third_party/src/glog-stamp/glog-patch
CMakeFiles/glog-complete: third_party/src/glog-stamp/glog-configure
CMakeFiles/glog-complete: third_party/src/glog-stamp/glog-build
CMakeFiles/glog-complete: third_party/src/glog-stamp/glog-install
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/Users/faraway/ps-project/feature_csci5570/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Completed 'glog'"
	/Applications/CLion.app/Contents/bin/cmake/mac/bin/cmake -E make_directory /Users/faraway/ps-project/feature_csci5570/cmake-build-debug/CMakeFiles
	/Applications/CLion.app/Contents/bin/cmake/mac/bin/cmake -E touch /Users/faraway/ps-project/feature_csci5570/cmake-build-debug/CMakeFiles/glog-complete
	/Applications/CLion.app/Contents/bin/cmake/mac/bin/cmake -E touch /Users/faraway/ps-project/feature_csci5570/cmake-build-debug/third_party/src/glog-stamp/glog-done

third_party/src/glog-stamp/glog-install: third_party/src/glog-stamp/glog-build
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/Users/faraway/ps-project/feature_csci5570/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Performing install step for 'glog'"
	cd /Users/faraway/ps-project/feature_csci5570/cmake-build-debug/third_party/src/glog-build && $(MAKE) install
	cd /Users/faraway/ps-project/feature_csci5570/cmake-build-debug/third_party/src/glog-build && /Applications/CLion.app/Contents/bin/cmake/mac/bin/cmake -E touch /Users/faraway/ps-project/feature_csci5570/cmake-build-debug/third_party/src/glog-stamp/glog-install

third_party/src/glog-stamp/glog-mkdir:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/Users/faraway/ps-project/feature_csci5570/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Creating directories for 'glog'"
	/Applications/CLion.app/Contents/bin/cmake/mac/bin/cmake -E make_directory /Users/faraway/ps-project/feature_csci5570/cmake-build-debug/third_party/src/glog
	/Applications/CLion.app/Contents/bin/cmake/mac/bin/cmake -E make_directory /Users/faraway/ps-project/feature_csci5570/cmake-build-debug/third_party/src/glog-build
	/Applications/CLion.app/Contents/bin/cmake/mac/bin/cmake -E make_directory /Users/faraway/ps-project/feature_csci5570/cmake-build-debug/third_party
	/Applications/CLion.app/Contents/bin/cmake/mac/bin/cmake -E make_directory /Users/faraway/ps-project/feature_csci5570/cmake-build-debug/third_party/tmp
	/Applications/CLion.app/Contents/bin/cmake/mac/bin/cmake -E make_directory /Users/faraway/ps-project/feature_csci5570/cmake-build-debug/third_party/src/glog-stamp
	/Applications/CLion.app/Contents/bin/cmake/mac/bin/cmake -E make_directory /Users/faraway/ps-project/feature_csci5570/cmake-build-debug/third_party/src
	/Applications/CLion.app/Contents/bin/cmake/mac/bin/cmake -E touch /Users/faraway/ps-project/feature_csci5570/cmake-build-debug/third_party/src/glog-stamp/glog-mkdir

third_party/src/glog-stamp/glog-download: third_party/src/glog-stamp/glog-gitinfo.txt
third_party/src/glog-stamp/glog-download: third_party/src/glog-stamp/glog-mkdir
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/Users/faraway/ps-project/feature_csci5570/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Performing download step (git clone) for 'glog'"
	cd /Users/faraway/ps-project/feature_csci5570/cmake-build-debug/third_party/src && /Applications/CLion.app/Contents/bin/cmake/mac/bin/cmake -P /Users/faraway/ps-project/feature_csci5570/cmake-build-debug/third_party/tmp/glog-gitclone.cmake
	cd /Users/faraway/ps-project/feature_csci5570/cmake-build-debug/third_party/src && /Applications/CLion.app/Contents/bin/cmake/mac/bin/cmake -E touch /Users/faraway/ps-project/feature_csci5570/cmake-build-debug/third_party/src/glog-stamp/glog-download

third_party/src/glog-stamp/glog-update: third_party/src/glog-stamp/glog-download
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/Users/faraway/ps-project/feature_csci5570/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "No update step for 'glog'"
	cd /Users/faraway/ps-project/feature_csci5570/cmake-build-debug/third_party/src/glog && /Applications/CLion.app/Contents/bin/cmake/mac/bin/cmake -E echo_append
	cd /Users/faraway/ps-project/feature_csci5570/cmake-build-debug/third_party/src/glog && /Applications/CLion.app/Contents/bin/cmake/mac/bin/cmake -E touch /Users/faraway/ps-project/feature_csci5570/cmake-build-debug/third_party/src/glog-stamp/glog-update

third_party/src/glog-stamp/glog-patch: third_party/src/glog-stamp/glog-download
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/Users/faraway/ps-project/feature_csci5570/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "No patch step for 'glog'"
	/Applications/CLion.app/Contents/bin/cmake/mac/bin/cmake -E echo_append
	/Applications/CLion.app/Contents/bin/cmake/mac/bin/cmake -E touch /Users/faraway/ps-project/feature_csci5570/cmake-build-debug/third_party/src/glog-stamp/glog-patch

third_party/src/glog-stamp/glog-configure: third_party/tmp/glog-cfgcmd.txt
third_party/src/glog-stamp/glog-configure: third_party/src/glog-stamp/glog-update
third_party/src/glog-stamp/glog-configure: third_party/src/glog-stamp/glog-patch
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/Users/faraway/ps-project/feature_csci5570/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_7) "Performing configure step for 'glog'"
	cd /Users/faraway/ps-project/feature_csci5570/cmake-build-debug/third_party/src/glog-build && /Applications/CLion.app/Contents/bin/cmake/mac/bin/cmake -DCMAKE_INSTALL_PREFIX:PATH=/Users/faraway/ps-project/feature_csci5570/cmake-build-debug -DWITH_GFLAGS=OFF -DBUILD_TESTING=OFF -DBUILD_SHARED_LIBS=OFF "-GCodeBlocks - Unix Makefiles" /Users/faraway/ps-project/feature_csci5570/cmake-build-debug/third_party/src/glog
	cd /Users/faraway/ps-project/feature_csci5570/cmake-build-debug/third_party/src/glog-build && /Applications/CLion.app/Contents/bin/cmake/mac/bin/cmake -E touch /Users/faraway/ps-project/feature_csci5570/cmake-build-debug/third_party/src/glog-stamp/glog-configure

third_party/src/glog-stamp/glog-build: third_party/src/glog-stamp/glog-configure
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/Users/faraway/ps-project/feature_csci5570/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_8) "Performing build step for 'glog'"
	cd /Users/faraway/ps-project/feature_csci5570/cmake-build-debug/third_party/src/glog-build && $(MAKE)
	cd /Users/faraway/ps-project/feature_csci5570/cmake-build-debug/third_party/src/glog-build && /Applications/CLion.app/Contents/bin/cmake/mac/bin/cmake -E touch /Users/faraway/ps-project/feature_csci5570/cmake-build-debug/third_party/src/glog-stamp/glog-build

glog: CMakeFiles/glog
glog: CMakeFiles/glog-complete
glog: third_party/src/glog-stamp/glog-install
glog: third_party/src/glog-stamp/glog-mkdir
glog: third_party/src/glog-stamp/glog-download
glog: third_party/src/glog-stamp/glog-update
glog: third_party/src/glog-stamp/glog-patch
glog: third_party/src/glog-stamp/glog-configure
glog: third_party/src/glog-stamp/glog-build
glog: CMakeFiles/glog.dir/build.make

.PHONY : glog

# Rule to build all files generated by this target.
CMakeFiles/glog.dir/build: glog

.PHONY : CMakeFiles/glog.dir/build

CMakeFiles/glog.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/glog.dir/cmake_clean.cmake
.PHONY : CMakeFiles/glog.dir/clean

CMakeFiles/glog.dir/depend:
	cd /Users/faraway/ps-project/feature_csci5570/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/faraway/ps-project/feature_csci5570 /Users/faraway/ps-project/feature_csci5570 /Users/faraway/ps-project/feature_csci5570/cmake-build-debug /Users/faraway/ps-project/feature_csci5570/cmake-build-debug /Users/faraway/ps-project/feature_csci5570/cmake-build-debug/CMakeFiles/glog.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/glog.dir/depend

