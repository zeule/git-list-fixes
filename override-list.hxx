#pragma once

#include <git2/types.h>

#include <filesystem>
#include <string>
#include <vector>

class CommitMessageOverrides {
public:
	CommitMessageOverrides();
	CommitMessageOverrides(git_repository& repo, const std::filesystem::path& filePath);

	const std::string* find(const git_oid& commit) const;

private:
	struct Entry {
		git_oid commit;
		std::string message;
	};

	std::vector<Entry> entries_;
};
