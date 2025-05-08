#include "config.hxx"

#include "utility.hxx"

#include <git2/config.h>
#include <git2/repository.h>

#include <utility>

Config::Config(git_repository& repo)
{
	LibgitError::check(git_repository_config(&config_, &repo));
}

Config::Config(Config&& other) noexcept
	: config_{std::exchange(other.config_, nullptr)}
{
}

Config& Config::operator=(Config&& other) noexcept
{
	if (config_) {
		git_config_free(config_);
	}
	config_ = other.config_;
	other.config_ = nullptr;
	return *this;
}

Config::~Config()
{
	if (config_) {
		git_config_free(config_);
	}
}

std::optional<std::string> Config::readString(const char* key) const
{
	std::optional<std::string> res;
	git_config_entry* entry;

	if (git_config_get_entry(&entry, config_, key)) {
		return res;
	}

	res = entry->value;
	git_config_entry_free(entry);
	return res;
}

namespace {
	int readMultiStringCallback(const git_config_entry* entry, void* payload)
	{
		static_cast<std::vector<std::string>*>(payload)->emplace_back(entry->value);
		return 0;
	}
}

std::vector<std::string> Config::readMultiString(const char* key) const
{
	std::vector<std::string> res;
	LibgitError::check(git_config_get_multivar_foreach(config_, key, nullptr, &readMultiStringCallback, &res));
	return res;
}
