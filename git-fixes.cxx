#include "git-fixes.hxx"

#include "commit.hxx"
#include "config.hxx"
#include "filters.hxx"
#include "tag-set.hxx"
#include "utility.hxx"

#include <git2/merge.h>
#include <git2/repository.h>
#include <git2/revparse.h>
#include <git2/revwalk.h>

#include <algorithm>
#include <cassert>
#include <iostream>
#include <memory>
#include <ranges>

template <typename T>
struct branch_merge_info: std::pair<std::vector<T>, std::vector<T>> {
	T merge_base;
};

using branch_merge_info_oid = branch_merge_info<git_oid>;

struct git_object_deleter {
	void operator()(git_object* object)
	{
		if (object) {
			git_object_free(object);
		}
	}
};

git_object* gitRevparseSingle(git_repository& repo, const char* spec)
{
	git_object* result;
	LibgitError::check(git_revparse_single(&result, &repo, spec));
	return result;
}

static branch_merge_info_oid load_commits(git_repository& repo, const std::string& first, const std::string& second)
{
	std::unique_ptr<git_object, git_object_deleter> firstObject{gitRevparseSingle(repo, first.c_str())};
	std::unique_ptr<git_object, git_object_deleter> secondObject{gitRevparseSingle(repo, second.c_str())};

	branch_merge_info_oid result;
	int r =
		git_merge_base(&result.merge_base, &repo, git_object_id(firstObject.get()), git_object_id(secondObject.get()));
	if (r) {
		throw std::runtime_error("Could not find merge base");
	}

	git_revwalk* walk;
	LibgitError::check(git_revwalk_new(&walk, &repo));

	LibgitError::check(git_revwalk_push(walk, git_object_id(firstObject.get())));
	LibgitError::check(git_revwalk_push(walk, &result.merge_base));
	LibgitError::check(git_revwalk_hide(walk, &result.merge_base));

	git_oid oid;
	while (!git_revwalk_next(&oid, walk)) {
		result.first.push_back(oid);
	}

	LibgitError::check(git_revwalk_reset(walk));
	LibgitError::check(git_revwalk_push(walk, git_object_id(secondObject.get())));
	LibgitError::check(git_revwalk_push(walk, &result.merge_base));
	LibgitError::check(git_revwalk_hide(walk, &result.merge_base));
	while (!git_revwalk_next(&oid, walk)) {
		result.second.push_back(oid);
	}

	git_revwalk_free(walk);
	return result;
}

std::vector<Reference> toReferencesArray(const std::vector<git_oid>& ids, Reference::Kind kind)
{
	std::vector<Reference> result;
	result.reserve(ids.size());
	std::ranges::transform(
		ids, std::back_inserter(result), [kind](const git_oid& id) { return Reference{.id = id, .kind = kind}; });
	return result;
}

bool containsAll(const std::vector<git_oid>& ids, const std::vector<git_oid>& references)
{
	return std::ranges::all_of(references, [&ids](const git_oid& ref) { return std::ranges::contains(ids, ref); });
}

bool containsAny(const std::vector<git_oid>& ids, const std::vector<git_oid>& references)
{
	return std::ranges::any_of(references, [&ids](const git_oid& ref) { return std::ranges::contains(ids, ref); });
}

void collectReferences(
	std::vector<git_oid>& destination, git_repository& repo, const std::vector<git_oid>& commits,
	const ReferenceExtractingFilter& filter)
{
	auto inserter{std::back_inserter(destination)};
	for (const git_oid& id: commits) {
		Commit c{repo, id};
		std::ranges::copy(filter.extract(c), inserter);
	}
}

std::vector<git_oid> removeReferencedCommits(
	git_repository& repo, const std::vector<git_oid>& commits, const ReferenceExtractingFilter& filter)
{
	std::vector<git_oid> referenses;
	auto inserter{std::back_inserter(referenses)};
	for (const git_oid& id: commits) {
		Commit c{repo, id};
		std::ranges::copy(filter.extract(c), inserter);
	}
	std::vector<git_oid> result;
	std::ranges::remove_copy_if(commits, std::back_inserter(result), [&referenses](const git_oid& id) {
		return std::ranges::contains(referenses, id);
	});
	return result;
}

void loadOptions(Options& options, git_repository& repo)
{
	Config config{repo};

	if (std::vector<std::string> fixes = config.readMultiString("list-fixes.fixesMatcher"); !fixes.empty()) {
		options.fixes_matchers = std::move(fixes);
	}

	if (std::optional<std::string> overrides = config.readString("list-fixes.overridesFile"); overrides.has_value()) {
		options.overrides_file = *overrides;
	}

	if (std::vector<std::string> tags = config.readMultiString("list-fixes.tagMatcher"); !tags.empty()) {
		options.tagMatchers = std::move(tags);
	}
}

std::vector<Commit> fixes(
	const Options& opts,
	git_repository& repo,
	const CommitMessageOverrides& overrides,
	const std::vector<git_oid>& blacklist)
{
	branch_merge_info_oid commits{load_commits(repo, opts.source, opts.revision)};

	RevertFilter revertFilter{repo, overrides};
	std::vector<git_oid> targetToRemove{blacklist};
	collectReferences(targetToRemove, repo, commits.second, revertFilter);
	std::vector<git_oid> targets{commits.second};
	targets.erase(
		std::remove_if(
			targets.begin(), targets.end(),
			[&targetToRemove](const git_oid& id) { return std::ranges::contains(targetToRemove, id); }),
		targets.end());

	// for the source branch we need only fixup commits, maybe filtered by other rules
	// CompoundFilter sourceFilters{filterForSources(opts, repo)};

	FixesFilter fixesFilter{opts.fixes_matchers, repo, overrides};
	std::map<std::string, std::vector<std::string>> tagSet =
		opts.tagSet.empty() ? std::map<std::string, std::vector<std::string>>{} : load_tag_set(opts.tagSet);
	TagMatcher tagsMatcher{
		tagSet.empty() ? std::vector<std::string>{} : opts.tagMatchers, std::move(tagSet), overrides};

	// some of the fixes might be already cherry-picked
	std::vector<git_oid>
		cherryPickedToTarget;
	collectReferences(cherryPickedToTarget, repo, commits.second, CherryPickedFilter{repo, overrides});

	std::vector<Commit> commitsToCherryPick;

	auto existsInTarget = [&](const git_oid& id) {
		// TODO the next two check are only for debugging, can/to be removed
		if (std::ranges::contains(cherryPickedToTarget, id)) {
			return true;
		}

		if (std::ranges::contains(commits.second, id)) {
			return true;
		}

		if (std::ranges::any_of(commitsToCherryPick, [&id](const Commit& c) { return c.id() == id; })) {
			return true;
		}

		if (std::ranges::contains(commits.first, id)) {
			return false;
		}

		return true;
	};

	for (const git_oid& id: std::ranges::reverse_view{commits.first}) {
		if (std::ranges::contains(blacklist, id) || std::ranges::contains(cherryPickedToTarget, id)) {
			continue;
		}
		Commit c{repo, id};
		// std::clog << "Analyzing " << c.logFormat() << std::endl;
		if (tagsMatcher(c)) {
			commitsToCherryPick.push_back(std::move(c));
			continue;
		}
		std::vector<Reference> references{toReferencesArray(fixesFilter.extract(c), Reference::Kind::Fixes)};
		std::ranges::copy(
			toReferencesArray(revertFilter.extract(c), Reference::Kind::Revert), std::back_inserter(references));
		if (references.empty()) {
			continue;
		}

		if (std::ranges::any_of(references, [&](const Reference& ref) { return existsInTarget(ref.id); })) {
			commitsToCherryPick.push_back(std::move(c));
		}
	}
	return commitsToCherryPick;
}
