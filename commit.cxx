#include "commit.hxx"

#include "note.hxx"
#include "utility.hxx"

#include <git2/commit.h>

#include <cassert>
#include <utility>

namespace {
	constexpr std::string_view clearMessageCommand{"{clear}\n"};
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

Commit::Commit(Commit&& other)
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

const git_oid& Commit::id() const
{
	return *git_commit_id(commit_);
}

std::string Commit::logFormat() const
{
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

	result += git_commit_message(commit_);

	return result;
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
