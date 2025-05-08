#pragma once

#include <git2/errors.h>
#include <git2/types.h>

#include <cassert>
#include <stdexcept>
#include <string>
#include <string_view>

void toLower(std::string& s);
std::string toLowerCopy(std::string_view s);

bool ishex(std::string_view s);

class LibgitError: public std::runtime_error {
public:
	LibgitError(int errorCode, const git_error* error);
	LibgitError(int error);

	/**
	 * @brief Throws LibgitError if error < 0
	 */
	static void check(int error);
};
