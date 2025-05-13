#pragma once

#include "commit.hxx"

#include <git2/types.h>

#include <filesystem>
#include <map>
#include <string>
#include <vector>

class CommitMessageOverrides;

struct Options {
	std::filesystem::path repo_path{"."};
	std::string revision{"HEAD"};
	std::string source{"master"};
	std::string author;
	std::string ignore_file;
	std::filesystem::path overrides_file;
	std::filesystem::path bl_file;
	std::filesystem::path bl_path_file;
	bool all_cmdline;
	bool me{false};
	bool my{false};
	bool all{true};
	bool match_all{false};
	bool group{true};
	bool stats{false};
	bool reverse{true};
	bool stable{true};
	bool write_bl{false};
	bool no_blacklist{false};
	bool parsable{false};
	bool patch{false};
	std::vector<std::string> path;
	std::vector<std::string> bl_path;
	std::vector<std::string> domains;
	std::vector<std::string> fixes_matchers{{R"(Fixes:\s([A-Fa-f0-9]+)\s\(".+"\))"}};
	std::vector<std::string> tagMatchers;
	std::filesystem::path tagSet;
};

void loadOptions(Options& options, git_repository& repo);

std::vector<Commit> fixes(
	const Options& opts,
	git_repository& repo,
	const CommitMessageOverrides& overrides,
	const std::vector<git_oid>& blacklist);
