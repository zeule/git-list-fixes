#pragma once

#include <git2/errors.h>
#include <git2/types.h>

#include <cassert>
#include <compare>
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

std::strong_ordering operator<=>(const git_oid& left, const git_oid& right);

inline bool operator==(const git_oid& left, const git_oid& right)
{
	return (left <=> right) == std::strong_ordering::equal;
}

std::string_view commitMessage(const git_commit& commit);

std::string& trimWhitespace(std::string& s);
