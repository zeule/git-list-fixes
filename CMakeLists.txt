cmake_minimum_required(VERSION 3.30 FATAL_ERROR)

project(git-list-fixes)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED On)
set(CMAKE_CXX_SCAN_FOR_MODULES Off)

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU|Clang")
	add_compile_options(-Wall;-Wextra;-Wshadow;-Wformat=2;-Wunused)
elseif("${CMAKE_CXX_COMPILER_ID}" EQUALS "MSVC")
	add_compile_options(-W4)
endif()

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
	config.hxx
	config.cxx
	filters.hxx
	filters.cxx
	git-fixes.hxx
	git-fixes.cxx
	note.hxx
	note.cxx
	reference.hxx
	tag-set.hxx
	tag-set.cxx
	utility.hxx
	utility.cxx
	main.cxx
)

target_link_libraries(git-list-fixes
	PRIVATE
	    libgit2::libgit2package CLI11::CLI11
)

install(
	TARGETS git-list-fixes
	DESTINATION bin
)
