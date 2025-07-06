#pragma once

#include <git2/types.h>

#include <string_view>

class Commit;

class Note {
public:
	Note(const Commit& commit, git_repository& repo);
	Note(Note&& other);
	~Note();

	std::string_view text() const;

private:
	git_note* note_{};
};
