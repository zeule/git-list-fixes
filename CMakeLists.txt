cmake_minimum_required(VERSION 4.0 FATAL_ERROR)

project(git-list-fixes)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED On)
set(CMAKE_CXX_SCAN_FOR_MODULES Off)

if (NOT WIN32)
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(libgit2 IMPORTED_TARGET GLOBAL libgit2>=1.9)
	add_library(libgit2::libgit2package ALIAS PkgConfig::libgit2)
else()
	find_package(libgit2 1.9 REQUIRED)
endif()

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
