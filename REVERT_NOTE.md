# Revert Note: PR #6 Commits Moved to Tea Branch

The following 7 commits were accidentally merged into `master` and have been reverted. The original feature work belongs on the `tea` branch.

## Commits removed from master

| SHA | Description |
|-----|-------------|
| `c106f4a` | Merge pull request #7 (revert of PR #6) |
| `1129e76` | Revert "Add /playlistadd, /shuffle, /randomsong jukebox commands restricted to CMs/VIPs/mods" |
| `a2412de` | Merge pull request #6 (feature branch) |
| `a25abda` | fix: restrict /playlistadd to area CMs, VIPs, and mods |
| `af5ed1f` | style: apply ternary for character name in cmdRandomSong |
| `2b10d91` | refactor: extract getPlayableSongs helper, fix audio extension filtering, add packet comment |
| `2e8fa0b` | feat: add /playlistadd, /shuffle, /randomsong commands with JUKEBOX permission |

## Status on `tea` branch

The `tea` branch already contains a more complete and advanced version of these features:

- `a9ecc5a` — Add /randomsong, /shuffle, and /playlistadd commands
- `1ffab2f` — Address code review: extract playableSongs helper
- `968154f` — Extend /playlistadd with custom URL queuing
- `66a4118` — Show both local-song and custom-URL examples in /help playlistadd

The `tea` branch is the correct home for these commands.
