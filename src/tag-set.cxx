#include "tag-set.hxx"

#include "utility.hxx"

#include <fstream>
#include <iostream>
#include <ranges>
#include <stdexcept>
#include <string_view>

namespace {
	namespace keys {
		constexpr std::string_view include{"Include="};
		constexpr std::string_view exclude{"Exclude="};
	}

	auto toString = [](auto&& r) -> std::string {
		return std::string{r.data(), r.size()};
	};

	auto split(const std::string& str, char delimiter) -> std::vector<std::string>
	{
		return str | std::ranges::views::split(delimiter) | std::ranges::views::transform(toString) |
		       std::ranges::to<std::vector<std::string>>();
	}
} // namespace

std::map<std::string, std::vector<std::string>> load_tag_set(const std::filesystem::path& filePath)
{
	std::ifstream file{filePath};
	if (!file) {
		throw std::runtime_error("could not read tag set from the file");
	}

	/*
	 * Format of the tag set file:
	 *
	 * [tag]
	 * Include=<semi-colon delimited list of tag values>
	 * Exclude=<semi-colon delimited list of tag values>
	 *
	 * Lines starting with '#' are ignored
	 */

	// currently we only return the Include values
	std::map<std::string, std::vector<std::string>> result;

	std::string tag;
	std::vector<std::string> includeList;

	std::string line;
	std::size_t currentLineNumber{};

	auto appendCurrentTag = [&]() {
		if (!tag.empty()) {
			result.emplace(tag, includeList);
		}
		includeList.clear();
	};

	while (std::getline(file, line)) {
		++currentLineNumber;
		trimWhitespace(line);
		if (line.starts_with('#') || (line.empty())) {
			continue;
		}
		if (line.size() > 2 && line.starts_with('[') && line.ends_with(']')) {
			appendCurrentTag();
			tag = line.substr(1, line.size() - 2);
		}
		if (line.starts_with(keys::include)) {
			includeList = split(line.substr(keys::include.size()), ';');
		}
		if (line.starts_with(keys::exclude)) {
			continue;
		}
		std::cerr << "Could not parse tag set file at " << filePath << ':' << currentLineNumber << std::endl;
		throw std::runtime_error("Tag set file format error");
	}

	if (!includeList.empty()) {
		appendCurrentTag();
	}
	return result;
}
