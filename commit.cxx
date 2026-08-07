#include "commit.hxx"

#include "note.hxx"
#include "utility.hxx"

#include <git2/commit.h>

#include <cassert>
#include <utility>

#include "git-list-fixes-config.hxx"
#ifndef Git_FOUND
#	include <chrono>
#	include <format>
#endif

namespace {
	constexpr std::string_view clearMessageCommand{"{clear}\n"};

#ifndef Git_FOUND
	std::string indentLines(const char* text, unsigned width)
	{
		std::string result;
		std::string indent(width, ' ');
		for (const char* p = text; *p; ++p) {
			result += *p;
			if (*p == '\n') {
				[[unlikely]] result += indent;
			}
		}
		return result;
	}
#endif
}

Commit::Commit(git_repository& repo, const git_oid& id)
{
	LibgitError::check(git_commit_lookup(&commit_, &repo, &id));
	assert(commit_);

	Note note{*this, repo};
	std::string_view noteText{trimWhitespace(note.text())};
	if (noteText.empty()) {
		message_ = std::string{git_commit_message(commit_)};
	} else {
		std::string_view::size_type lastClear = noteText.rfind(clearMessageCommand);
		if (lastClear == std::string_view::npos) {
			message_ = std::string{git_commit_message(commit_)} + std::string{noteText};
		} else {
			message_ = std::string{noteText.substr(lastClear + clearMessageCommand.size())};
		}
	}
}

Commit::Commit(Commit&& other) noexcept
	: commit_{std::exchange(other.commit_, nullptr)}
	, message_{std::move(other.message_)}
{
}

Commit::~Commit()
{
	if (commit_) {
		git_commit_free(commit_);
	}
}

Commit& Commit::operator=(Commit&& other) noexcept
{
	std::swap(commit_, other.commit_);
	return *this;
}

const git_oid& Commit::id() const
{
	return *git_commit_id(commit_);
}

std::string Commit::logFormat() const
{
#ifdef Git_FOUND
	std::string command = "git log --color=always -1 ";
	std::size_t id_pos = command.size();
	command.resize(command.size() + 40);
	git_oid_fmt(&command[id_pos], git_commit_id(commit_));
	return launch(command.c_str());
#else
	// commit b178cd50e13f4dbe50fa4a8759f46eeec58585a2
	// Author: Eugene Shalygin <eugene.shalygin@gmail.com>
	// Date: Mon Apr 21 10:01:12 2025 +0200

	// Fourth

	std::string result;
	result += "commit ";
	result.resize(result.size() + 40);
	git_oid_fmt(&result[7], git_commit_id(commit_));
	result += "\nAuthor: ";

	const git_signature* signature = git_commit_author(commit_);
	result += signature->name;
	result += " <";
	result += signature->email;
	result += ">\n";

	std::chrono::sys_seconds commitTime{std::chrono::seconds{git_commit_time(commit_)}};
	int timeZoneOffset = git_commit_time_offset(commit_);

	result +=
		std::format("Date:   {:%a %b %d %T %Y} {:+03}{:02}\n\n", commitTime, timeZoneOffset / 60, timeZoneOffset % 60);

	result += std::format("{:{}}{}", "", 4, indentLines(git_commit_message(commit_), 4));

	return result;
#endif
}

std::string_view Commit::autorEmail() const
{
	return {git_commit_author(commit_)->email};
}

CommitWithReferences::CommitWithReferences(git_repository& repo, const git_oid& id, std::vector<Reference> references)
	: Commit(repo, id)
	, references_{std::move(references)}
{
}

CommitWithReferences::CommitWithReferences(Commit&& commit, std::vector<Reference> references)
	: Commit(std::move(commit))
	, references_{std::move(references)}
{
}
