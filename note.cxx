#include "note.hxx"

#include "commit.hxx"

#include <git2/notes.h>

#include <utility>

Note::Note(const Commit& commit, git_repository& repo)
{
	if (git_note_read(&note_, &repo, nullptr, &commit.id())) {
		note_ = nullptr;
	}
}

Note::Note(Note&& other)
	: note_{std::exchange(other.note_, nullptr)}
{
}

Note::~Note()
{
	if (note_) {
		git_note_free(note_);
	}
}

std::string_view Note::text() const
{
	if (!note_) {
		return {};
	}
	const char* message = git_note_message(note_);
	return {message};
}
