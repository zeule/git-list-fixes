# git list-fixes

Lists fixup commits from the source branch to be cherry-picked onto the target branch.

Supports multiple way to figure out which commits are the fixup ones, and what do the fix.

## Example workflow

Suppose you develop in the master branch, and release from the releases branch. Now, you find a bug in an old commit A and fix it in the master with commit B. You check if the commit A is part of the releases branch (`git branch --contains A`). But it might have been cherry-picked onto branch releases. You check that as well. Yes, A is part of the releases branch. This means you need to cherry-pick B onto releases as well.

Now, `git list-fixes` automates that and similar workflows.

## Fixup commits

What are the fixup commits? Those are commits that reference other commits via message or [note][git-notes]. These references can, for example, follow the Linux kernel style:  

 ```
 Fixes: fdc242155b82 ("The excellent feature")
 ```
 
or any other format you can write a regular expression to extract the commit SHA from. The Linux kernel format is supported by the default configuration.

Another example of fixup commit reference is what `git revert` writes:  
```
This reverts commit ac1bfde19fd74ab4eded69894619661bca1e5585
```

## How it works

Given a target revision (default `HEAD`) and a source revision (default `master`), `git list-fixes`:

1. Finds the merge base of the two revisions and walks each branch's history since that point.
2. Identifies "fixup" commits on the source branch — commits whose message    contains a `Fixes: <sha> ("...")`-style reference (configurable), or commits that `git revert` another commit.
3. Keeps only fixes whose referenced commit is present on the target branch, and skips fixes that are already cherry-picked into the target (detected via `(cherry picked from commit ...)` trailers) or that appear on an explicit blacklist.
4. Reconciles fixes and reverts: if both a commit and everything that reverts it are selected, both are dropped from the result, since they cancel out.
5. Optionally matches commits against a user-defined tag set instead of (or in addition to) the `Fixes:` heuristic, useful for projects that track fixes with their own note/tag conventions.
6. Prints the resulting commits — as a `git log`-style listing, grouped by author, or as a ready-to-run sequence of `git cherry-pick` commands.

## Usage

```sh
git-list-fixes [revspec] [path] [options]
```

Run from inside (or point `--repo` at) the git repository you want to inspect.

If you installed the binary to location where `git` can find it, as with other `git` commands, this one can be invoked as `git list-fixes`.

### Examples

Show fixes on `master` that are missing from the current `HEAD`, grouped by
author:

```sh
git list-fixes
```

Check what's missing from a specific release branch, sourced from `master`:

```sh
git list-fixes release/2.4 --source master
```

Only show fixes for commits you authored, printed as cherry-pick commands
ready to run:

```sh
git list-fixes --me --script
```

## Configuration

The configuration is read from the `git` configuration system, you can use `git config` to store global and per-repository settings.

The main setting is the list of regular expressions used to extract fixup references. It is read from `list-fixes.fixesMatcher` key as a multi-valued string:

```
[list-fixes]
    fixesMatcher = Fixes:\\s([A-Fa-f0-9]+)\\s\\(".+"\\)
    fixesMatcher = [Aa]mend\\s([A-Fa-f0-9]+)
    tagMatcher = MyTag:\\s(\\S+)
```


[git-notes]: https://git-scm.com/docs/git-notes
