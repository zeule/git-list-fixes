#pragma once

#include "reference.hxx"

#include <git2/types.h>

#include <string>
#include <vector>

class Commit {
public:
	Commit(git_repository& repo, const git_oid& id);
	Commit(Commit&& other);
	~Commit();

	operator const git_commit&() const
	{
		return *commit_;
	}

	const git_oid& id() const;

	const std::string& message() const { return message_; }

	std::string logFormat() const;

	std::string_view autorEmail() const;

private:
	git_commit* commit_;
	std::string message_;
};

class CommitWithReferences: public Commit {
public:
	CommitWithReferences(git_repository& repo, const git_oid& id, std::vector<Reference> references);
	CommitWithReferences(Commit&& commit, std::vector<Reference> references);

	const std::vector<Reference>& references() const { return references_; }

private:
	std::vector<Reference> references_;
};
