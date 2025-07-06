#include "filters.hxx"

#include "config.hxx"
#include "utility.hxx"

#include <git2/commit.h>
#include <git2/config.h>
#include <git2/repository.h>
#include <git2/revparse.h>

#include <algorithm>
#include <cassert>
#include <format>
#include <regex>
#include <stdexcept>
#include <vector>

namespace {
	constexpr std::string_view revertMessage{"This reverts commit "};

	// (cherry picked from commit b178cd50e13f4dbe50fa4a8759f46eeec58585a2)
	constexpr std::string_view cherryPickedMessage{"(cherry picked from commit "};

	std::vector<std::regex> makeMatchers(const std::vector<std::string>& expressions)
	{
		std::vector<std::regex> res;
		res.reserve(expressions.size());
		for (const std::string& exp: expressions) {
			res.emplace_back(exp, std::regex::ECMAScript);
		}
		return res;
	}
} // namespace

class AuthorFilter: public CommitFilter {
public:
	AuthorFilter(const std::string& author)
		: author_{author}
	{
	}

	bool operator()(const Commit& commit) const override { return author_ == commit.autorEmail(); }

	const std::string& author_;
};

FixesFilter::FixesFilter(const std::vector<std::string>& matchExpressions, git_repository& repo)
	: matchers_{makeMatchers(matchExpressions)}
	, repo_{repo}
{
	for (const std::tuple<const std::regex&, const std::string&> rs: std::views::zip(matchers_, matchExpressions)) {
		if (std::get<0>(rs).mark_count() < 1) {
			throw WrongMatcherRegex{
				std::format("Expected at least one capture group in fixes matching expression '{}'", std::get<1>(rs))};
		}
	}
}

bool FixesFilter::operator()(const Commit& commit) const
{
	auto end{std::cregex_iterator()};
	std::string_view message{commitMessage(commit)};
	for (const std::regex& matcher: matchers_) {
		for (auto iter = std::cregex_iterator(message.data(), message.data() + message.size(), matcher); iter != end;
		     ++iter) {
			return true;
		}
	}
	return false;
	/*
	        std::cout << "Found " << std::distance(words_begin, words_end) << " words:\n";

	        for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
	            std::smatch match = *i;
	            std::string match_str = match.str();
	            std::cout << match_str << '\n';
	        }
	*/
}

std::vector<git_oid> FixesFilter::extract(const Commit& commit) const
{
	std::vector<git_oid> result;

	std::string_view message{commitMessage(commit)};

	for (const std::regex& matcher: matchers_) {
		const char* msg = message.data();
		for (std::cmatch match; std::regex_search(msg, match, matcher);) {
			assert(match.length(1) > 0);
			git_object* obj;
			int error = git_revparse_single(&obj, &repo_, match[1].str().c_str());
			if (!error) {
				result.push_back(*git_object_id(obj));
				git_object_free(obj);
			}
			msg = match.suffix().first;
		}
	}
	return result;
}

bool StdGitMessageExtractor::operator()(const Commit& commit) const
{
	std::string_view message{commitMessage(commit)};
	return message.find(messageStart_) != std::string_view::npos;
}

std::vector<git_oid> StdGitMessageExtractor::extract(const Commit& commit) const
{
	std::vector<git_oid> result;
	std::string_view message{commitMessage(commit)};

	auto lineStart = message.find(messageStart_);
	if (lineStart != std::string_view::npos && message.size() >= lineStart + messageStart_.size() + 40) {
		std::string_view id = message.substr(lineStart + messageStart_.size(), 40);
		if (ishex(id)) {
			git_oid oid{0};
			LibgitError::check(git_oid_fromstr(&oid, id.data()));
			result.push_back(oid);
		}
	}
	return result;
}

StdGitMessageExtractor::StdGitMessageExtractor(git_repository& repo, std::string_view messageStart)
	: repo_{repo}
	, messageStart_{messageStart}
{
}

RevertFilter::RevertFilter(git_repository& repo)
	: StdGitMessageExtractor{repo, revertMessage}
{
}

CherryPickedFilter::CherryPickedFilter(git_repository& repo)
	: StdGitMessageExtractor{repo, cherryPickedMessage}
{
}

TagMatcher::TagMatcher(
	const std::vector<std::string>& matchExpressions, std::map<std::string, std::vector<std::string>> targetTags)
	: matchers_{makeMatchers(matchExpressions)}
	, targetTags_{std::move(targetTags)}
{
	for (const std::tuple<const std::regex&, const std::string&> rs: std::views::zip(matchers_, matchExpressions)) {
		if (std::get<0>(rs).mark_count() < 2) {
			throw WrongMatcherRegex{
				std::format("Expected at least two capture group in tag matching expression '{}'", std::get<1>(rs))};
		}
	}
}

bool TagMatcher::operator()(const Commit& commit) const
{
	std::string_view message{commitMessage(commit)};

	for (const std::regex& matcher: matchers_) {
		const char* msg = message.data();
		for (std::cmatch match; std::regex_search(msg, match, matcher);) {
			assert(match.length(1) > 0);
			auto targetTagsIt = targetTags_.find(match[1].str());
			if (targetTagsIt == targetTags_.end()) {
				continue;
			}
			if (std::ranges::contains(targetTagsIt->second, match[2].str())) {
				return true;
			}
			msg = match.suffix().first;
		}
	}
	return false;
}

bool CompoundFilter::operator()(const Commit& commit) const
{
	return std::ranges::all_of(filters_, [&commit](const auto& filter) { return (*filter)(commit); });
}

std::unique_ptr<ReferenceExtractingFilter> fixesFilter(const Options& opts, git_repository& repo)
{
	return std::make_unique<FixesFilter>(opts.fixes_matchers, repo);
}

CompoundFilter filterForSources(const Options& opts, git_repository& repo)
{
	if (opts.fixes_matchers.empty()) {
		throw std::runtime_error("Fixes matchers are not defined");
	}

	std::string author = opts.author;
	if (opts.my) {
		Config cfg{repo};
		std::optional<std::string> myEmail = cfg.readString("user.email");
		if (myEmail) {
			author = *myEmail;
		}
	}
	CompoundFilter result;
	if (!author.empty()) {
		result.push_back(std::make_unique<AuthorFilter>(opts.author));
	}

	return result;
}
