#pragma once

#include "git-fixes.hxx"

#include <git2/types.h>

#include <map>
#include <memory>
#include <regex>
#include <stdexcept>
#include <string_view>

class Commit;

class WrongMatcherRegex: public std::runtime_error {
	using std::runtime_error::runtime_error;
};

struct CommitFilter {
	virtual ~CommitFilter() = default;
	virtual bool operator()(const Commit& commit) const = 0;
};

struct ReferenceExtractingFilter: CommitFilter {
	virtual std::vector<git_oid> extract(const Commit& commit) const = 0;
};

class StdGitMessageExtractor: public ReferenceExtractingFilter {
	using base = ReferenceExtractingFilter;

public:
	bool operator()(const Commit& commit) const override;
	std::vector<git_oid> extract(const Commit& commit) const override;

protected:
	StdGitMessageExtractor(git_repository& repo, std::string_view messageStart);

private:
	git_repository& repo_;
	std::string_view messageStart_;
};

class RevertFilter: public StdGitMessageExtractor {
public:
	RevertFilter(git_repository& repo);
};

class CherryPickedFilter: public StdGitMessageExtractor {
public:
	CherryPickedFilter(git_repository& repo);
};

/**
 * @brief Extracts Fixes: like references
 */
class FixesFilter: public ReferenceExtractingFilter {
	using base = ReferenceExtractingFilter;

public:
	FixesFilter(const std::vector<std::string>& matchExpressions, git_repository& repo);
	bool operator()(const Commit& commit) const override;
	std::vector<git_oid> extract(const Commit& commit) const override;

private:
	std::vector<std::regex> matchers_;
	git_repository& repo_;
};

/**
 * @brief Matches commites that are
 */
class TagMatcher: CommitFilter {
public:
	TagMatcher(
		const std::vector<std::string>& matchExpressions, std::map<std::string, std::vector<std::string>> targetTags);
	bool operator()(const Commit& commit) const override;

private:
	std::vector<std::regex> matchers_;
	std::map<std::string, std::vector<std::string>> targetTags_;
};

class CompoundFilter: public CommitFilter {
public:
	void push_back(std::unique_ptr<CommitFilter> filter) { filters_.push_back(std::move(filter)); }

	bool operator()(const Commit& commit) const override;

private:
	std::vector<std::unique_ptr<CommitFilter>> filters_;
};

CompoundFilter filterForSources(const Options& opts, git_repository& repo);
