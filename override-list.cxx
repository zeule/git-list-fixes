#include "override-list.hxx"

#include "commit.hxx"
#include "utility.hxx"

#include <git2/commit.h>
#include <git2/revparse.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>

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

CommitMessageOverrides::CommitMessageOverrides() {}

CommitMessageOverrides::CommitMessageOverrides(git_repository& repo, const std::filesystem::path& filePath)
{
	/*
	 * Format of the override file is as follows:
	 *
	 * [commit-sha1]
	 * Replacement for th commit message. May
	 * be multiline.
	 *
	 * May contain empty lines.
	 *
	 * {commit-sha}
	 * Appends to the commit message.
	 */
	std::ifstream file{filePath};

	std::string line;
	std::size_t currentLineNumber{};

	git_oid foundId;
	std::string message;
	bool inMessage{};

	while (std::getline(file, line)) {
		++currentLineNumber;
		trim(line);
		if (line.starts_with('#') || (line.empty() && !inMessage)) {
			continue;
		}

		if (line.size() > 2 && (line.starts_with('[') && line.ends_with(']')) ||
		    (line.starts_with('{') && line.ends_with('}'))) {
			// reached new commit
			if (inMessage) {
				trim(message);
				entries_.push_back(Entry{.commit = foundId, .message = message});
				message.clear();
			}

			if (git_oid_fromstrn(&foundId, line.data() + 1, line.size() - 2)) {
				// maybe it is a shortended one?
				line.back() = 0;
				git_object* object;
				if (git_revparse_single(&object, &repo, line.data() + 1)) {
					std::cerr << "Could not parse commit id at " << filePath << ':' << currentLineNumber << std::endl;
					throw std::runtime_error("Override file format error");
				}
				foundId = *git_object_id(object);
				git_object_free(object);
			}
			inMessage = true;
			if (line.starts_with('{')) {
				Commit commit{repo, foundId};
				message = commit.message();
			}
		} else {
			message += '\n';
			message += line;
		}
	}
	if (inMessage) {
		trim(message);
		entries_.push_back(Entry{.commit = foundId, .message = std::move(message)});
	}
}

const std::string* CommitMessageOverrides::find(const git_oid& commit) const
{
	auto it = std::ranges::find_if(entries_, [&commit](const Entry& e) { return e.commit == commit; });
	return it == entries_.end() ? nullptr : &it->message;
}
