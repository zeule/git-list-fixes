#pragma once

#include <string_view>

#cmakedefine Git_FOUND

constexpr std::string_view GIT_EXECUTABLE{"@GIT_EXECUTABLE@"};
