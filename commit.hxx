#pragma once

#include <git2/types.h>

#include <cassert>
#include <string>
#include <vector>

class Commit {
public:
	Commit(git_repository& repo, const git_oid& id);
	Commit(Commit&& other);
	~Commit();

	operator const git_commit&() const
	{
		assert(commit_);
		return *commit_;
	}

	const git_oid& id() const;
	std::string logFormat() const;

private:
	git_commit* commit_;
};

struct Reference {
	enum class Kind { Fixes, Revert };

	git_oid id;
	Kind kind;
};

class CommitWithReferences: public Commit {
public:
	CommitWithReferences(git_repository& repo, const git_oid& id, std::vector<Reference> references);
	CommitWithReferences(Commit&& commit, std::vector<Reference> references);

	const std::vector<Reference>& references() const { return references_; }

private:
	std::vector<Reference> references_;
};
