#include "git-fixes.hxx"
#include "utility.hxx"

#include <CLI/App.hpp>
#include <CLI/Config.hpp>
#include <CLI/Formatter.hpp>
#include <CLI/Validators.hpp>
#include <git2/global.h>
#include <git2/repository.h>

#include <iostream>

struct CommitSHAValidator: CLI::Validator {
	CommitSHAValidator()
		: CLI::Validator("SHA")
	{
		func_ = [](std::string& value) {
			if (!ishex(toLowerCopy(value))) {
				return std::string("Invalid SHA");
			}
			return std::string{};
		};
	}
};

std::vector<git_oid> blacklist;

struct libgit2 {
	libgit2() { git_libgit2_init(); }

	~libgit2() { git_libgit2_shutdown(); }
};

struct git_repo_deleter {
	void operator()(git_repository* repo)
	{
		if (repo) {
			git_repository_free(repo);
		}
	}
};

git_repository* repository_open(const std::filesystem::path& repo)
{
	git_repository* result;
	LibgitError::check(git_repository_open(&result, repo.c_str()));
	return result;
}

int main(int argc, char** argv)
{
	Options opts;
	CLI::App app;
	CLI::Option_group* output_options = app.add_option_group("output", "Output controls");

	app.add_option("revspec", opts.revision);
	app.add_option("path", opts.path);

	app.add_option("--repo,-r", opts.repo_path, "Path to git repo")->capture_default_str()->check(CLI::ExistingPath);
	app.add_option("--source", opts.source, "Source revspec")->capture_default_str();
	app.add_flag("--reverse", opts.reverse, "Sort fixes in reverse order");
	app.add_flag("--stable,!--no-stable", opts.stable, "Show only commits with a stable-tag")->capture_default_str();
	app.add_option(
		   "--fixes-matchers", opts.fixes_matchers,
		   "Regular expressions to extract fixup commits. First capture group must capture commit-id")
		->capture_default_str();
	// app.add_option("--file,-f", opts.fixes_file, "Read commit-list from file")->check(CLI::ExistingFile);
	output_options->add_flag("--stats,-s", opts.stats, "Print some statistics at the end");
	output_options->add_flag("--patch", opts.patch, "Print patch-filename the fix is for (if available)");
	output_options->add_flag("--parsable,-p", opts.parsable, "Print machine readable output");
	output_options->add_flag("--grouping,!--no-grouping", opts.group, "Group fixes by committer")
		->capture_default_str();

	app.add_option(
		"--ignore-file", opts.ignore_file,
		"Specify a file with commits to be added to the commit-list, but not checked for pending fixes. Use this "
		"to ignore fixes already in the tree");
	// app.add_option("--data-base,-d", opts.db, "Select specific data-base (set file with fixes.<db>.file)");
	app.add_option(
		"--domains", opts.domains,
		"If the author of the fix has an email address with one of the domains specified here, it gets the fix "
		"assigned directly.");
	app.add_option("-b,--blacklist", opts.bl_file, "Read blacklist from file")->check(CLI::ExistingFile);
	app.add_flag("--no-blacklist", opts.no_blacklist, "Also show blacklisted commits");
	app.add_option_function(
		   "--Blacklist,-B", std::function{[&opts](const std::string& value) {
			   opts.write_bl = true;
			   git_oid id;
			   git_oid_fromstr(&id, value.c_str());
			   blacklist.push_back(id);
		   }},
		   "Add commit to blacklist")
		->check(CommitSHAValidator());
	app.add_option("--path-blacklist", opts.bl_path_file, "Filename containing the path-blacklist");
	app.add_flag("--match-all,-m", opts.match_all, "Match against everything that looks like a git commit-id");

	CLI::Option* optCommitter = app.add_option_function(
		"--author", std::function{[&opts](const std::string& value) {
			opts.author = value;
			opts.all = false;
			opts.all_cmdline = true;
		}},
		"Show only fixes by given author email");
	CLI::Option* optMe = app.add_flag("--me", opts.my, "Show only fixes for my commits");
	CLI::Option* optAll = app.add_flag_function(
		"--all,-a", std::function{[&opts](std::int64_t) {
			opts.all = true;
			opts.all_cmdline = true;
		}},
		"Show all potential fixes");
	app.add_flag("--my", opts.my, "Show only the fixes I committed");

	optCommitter->excludes(optMe)->excludes(optAll);
	optMe->excludes(optCommitter)->excludes(optAll);
	optAll->excludes(optCommitter)->excludes(optMe);

	CLI11_PARSE(app, argc, argv);

	libgit2 libgit;
	std::unique_ptr<git_repository, git_repo_deleter> repo{repository_open(opts.repo_path)};
	loadOptions(opts, *repo);
	std::vector<CommitWithReferences> fixupCommits{fixes(opts, *repo, blacklist)};

	for (const CommitWithReferences& c: fixupCommits) {
		std::cout << c.logFormat() << std::endl;
	}

	return 0;
}
