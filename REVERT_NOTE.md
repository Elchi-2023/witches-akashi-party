# Revert Note: PR #6 Commits Moved to Tea Branch

The following 7 commits were accidentally merged into `master` (via PR #6) and then reverted (via PR #7).
The original feature work belongs on the `tea` branch.

## The 7 commits

| SHA | Description |
|-----|-------------|
| `c106f4a` | Merge pull request #7 (revert of PR #6) |
| `1129e76` | Revert "Add /playlistadd, /shuffle, /randomsong jukebox commands restricted to CMs/VIPs/mods" |
| `a2412de` | Merge pull request #6 (feature branch) |
| `a25abda` | fix: restrict /playlistadd to area CMs, VIPs, and mods |
| `af5ed1f` | style: apply ternary for character name in cmdRandomSong |
| `2b10d91` | refactor: extract getPlayableSongs helper, fix audio extension filtering, add packet comment |
| `2e8fa0b` | feat: add /playlistadd, /shuffle, /randomsong commands with JUKEBOX permission |

## Current state of `master`

PR #7 fully reverted PR #6, so `master`'s file contents are identical to `c01662b` (the commit before
PR #6 landed). The 7 commits have **zero net effect** on the codebase.

To completely remove these commits from `master`'s history, a force-push is required:

```bash
git push --force origin c01662b:master
```

## Status on `tea` branch

The `tea` branch already contains a more complete and advanced implementation of these features:

- `a9ecc5a` — Add /randomsong, /shuffle, and /playlistadd commands with /help entries
- `1ffab2f` — Address code review: extract playableSongs helper, remove fragile string comparison
- `968154f` — Extend /playlistadd with custom URL queuing; update /help examples
- `66a4118` — Show both local-song and custom-URL examples in /help playlistadd

All three commands (`/playlistadd`, `/shuffle`, `/randomsong`) are fully registered and implemented on
the `tea` branch. The `tea` branch is the correct home for these commands.
