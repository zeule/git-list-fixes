#pragma once

#include <git2/types.h>

struct Reference {
	enum class Kind { Fixes, Revert };

	git_oid id;
	Kind kind;
};
