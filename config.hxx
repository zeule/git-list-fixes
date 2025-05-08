#pragma once

#include <git2/types.h>

#include <optional>
#include <string>
#include <vector>

class Config {
public:
	Config(git_repository& repo);
	Config(Config&& other) noexcept;
	Config& operator=(Config&&) noexcept;
	~Config();

	Config(const Config&) = delete;
	Config& operator=(const Config&) = delete;

	std::optional<std::string> readString(const char* key) const;
	std::vector<std::string> readMultiString(const char* key) const;

private:
	git_config* config_;
};
