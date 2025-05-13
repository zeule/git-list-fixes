#include "utility.hxx"

#include <git2/commit.h>
#include <git2/config.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <format>

namespace {
	const char* ws = " \t\n\r\f\v";

	inline std::string& rtrim(std::string& s, const char* t = ws)
	{
		s.erase(s.find_last_not_of(t) + 1);
		return s;
	}

	inline std::string& ltrim(std::string& s, const char* t = ws)
	{
		s.erase(0, s.find_first_not_of(t));
		return s;
	}

	inline std::string& trim(std::string& s, const char* t = ws)
	{
		return ltrim(rtrim(s, t), t);
	}
} // namespace

void toLower(std::string& s)
{
	std::ranges::transform(s, s.begin(), ::tolower);
}

std::string toLowerCopy(std::string_view s)
{
	std::string result{s};
	toLower(result);
	return result;
}

bool ishex(std::string_view s)
{
	return std::ranges::all_of(s, ::isxdigit);
}

LibgitError::LibgitError(int errorCode, const git_error* error)
	: std::runtime_error(std::format("libgit2 error {}/{}: {}", errorCode, error->klass, error->message))
{
}

LibgitError::LibgitError(int error)
	: LibgitError(error, git_error_last())
{
}

void LibgitError::check(int error)
{
	if (error < 0) {
		throw LibgitError(error);
	}
}

std::strong_ordering operator<=>(const git_oid& left, const git_oid& right)
{
	int r = git_oid_cmp(&left, &right);
	return r > 0 ? std::strong_ordering::greater : (r < 0 ? std::strong_ordering::less : std::strong_ordering::equal);
}

std::string_view commitMessage(const git_commit& commit)
{
	const char* message = git_commit_message(&commit);
	return {message, std::strlen(message)};
}

std::string& trimWhitespace(std::string& s)
{
	return trim(s);
}
