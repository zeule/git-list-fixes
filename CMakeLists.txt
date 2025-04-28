cmake_minimum_required(VERSION 4.0 FATAL_ERROR)

project(git-list-fixes)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED On)
set(CMAKE_CXX_SCAN_FOR_MODULES Off)

find_package(libgit2 1.9 REQUIRED)
find_package(CLI11 2.5.0 REQUIRED)

add_executable(git-list-fixes
	commit.hxx
	commit.cxx
	filters.hxx
	filters.cxx
	git-fixes.hxx
	git-fixes.cxx
	utility.hxx
	utility.cxx
	main.cxx
)
target_link_libraries(git-list-fixes PRIVATE libgit2::libgit2package)

install(
	TARGETS git-list-fixes
	DESTINATION bin
)
