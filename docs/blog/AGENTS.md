# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a custom Anki-like spaced repetition system built around the FSRS (Free Spaced Repetition Scheduler) algorithm. Unlike traditional Anki, this system integrates directly with markdown notes using UUIDs embedded in comments, allowing seamless integration with existing note-taking workflows.

The system consists of:
- **Rust CLI tool** (`fsrs/`) - Implements FSRS algorithm and manages review scheduling
- **Shell scripts** - Provide user interface for reviewing and searching cards
- **JSON database** (`anki.json`) - Stores review history
- **Markdown notes** - Any markdown file can contain flashcards by adding UUID comments

## Architecture

### Core Components

1. **FSRS Algorithm Implementation** (`fsrs/src/main.rs`)
   - Uses the `fsrs-rs` library (v5.2.0) for spaced repetition scheduling
   - Stores review data in JSON format (`anki.json`) instead of SQLite
   - Each flashcard is identified by a UUID
   - Review history tracks timestamp and rating (1=Again, 2=Hard, 3=Good, 4=Easy)
   - Calculates memory state (Stability, Difficulty, Retrievability) for each card

2. **Data Model**
   - Database is a flat JSON file with UUID keys mapping to arrays of log entries
   - Each log entry contains: `time` (RFC3339 timestamp) and `rating` (1-4)
   - Old format compatibility: converts boolean `ok` field to ratings (true→3, false→1)
   - Reviews are stored in reverse chronological order (most recent first)

3. **Flashcard Format**
   - Flashcards are markdown headers with UUID comments:
     ```markdown
     ## Question/Title
     <!-- uuid-here -->
     Answer content goes here
     ```
   - The header is the prompt, content below is what to recall
   - UUID must be immediately after the header (on the next line)

4. **Review Interface** (`anki.sh`)
   - Scans markdown files for UUID patterns using ripgrep
   - Builds a cache mapping UUIDs to file locations (`/tmp/anki_cache/uuid_mapping.cache`)
   - Uses `gum` for styled terminal UI and `fzf` for selection
   - Opens neovim at the correct line for reviewing content
   - Validates that each UUID has a proper markdown header above it

## Development Commands

### Building the Rust Tool

```bash
cd fsrs
cargo build --release
# Binary location: fsrs/target/release/anki-fsrs
# Debug binary: fsrs/target/debug/anki-fsrs
```

### Running the Review System

```bash
# Refresh cache and import new cards from markdown files
./anki.sh -r

# Start review session (review due cards)
./anki.sh

# Show daily statistics
./anki.sh -s

# Search all cards with fzf
./anki-fzf.sh
```

### Rust CLI Tool Commands

```bash
cd fsrs
cargo run -- --due                           # List top 10 cards due for review
cargo run -- --insert <UUID> <rating>        # Add review (A/H/G/E or 1/2/3/4)
cargo run -- --check <UUID>                  # Check if UUID exists
cargo run -- --when <UUID>                   # Query next review time
cargo run -- --delete <UUID>                 # Delete a card
cargo run -- --inspect <UUID>                # Show review history
cargo run -- --stats                         # Daily statistics
cargo run -- --recent-decks                  # Cards added in past week
```

### Testing

No formal test suite. Manual testing workflow:
1. Add a test UUID to a markdown file
2. Run `./anki.sh -r` to import
3. Review the card with `./anki.sh`
4. Verify with `cargo run -- --inspect <UUID>`

## Key Technical Details

### FSRS Algorithm Parameters
- Uses `DEFAULT_PARAMETERS` from fsrs-rs library
- Desired retention: 0.9 (90% probability of recall)
- Decay constant: `FSRS6_DEFAULT_DECAY`
- Memory state calculates: Stability (S), Difficulty (D), Retrievability (R)

### Due Card Prioritization
Cards are scored by: `(1 - retrievability) + (days_overdue / 30.0)`
- Lower retrievability = higher priority
- More overdue = higher priority
- Top 10 highest scores are shown for review

### Cache Management
- Cache location: `/tmp/anki_cache/uuid_mapping.cache`
- Format: `UUID:file_path:line_number`
- Rebuilt on each `anki.sh -r` run
- Auto-deletes UUIDs not found in cache

### UUID Pattern
- Standard UUID format: `[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}`
- Must be in HTML comment: `<!-- uuid -->`
- Must be preceded by a markdown header line (starts with `#`)

## Environment Setup

### Required Tools
- Rust toolchain (see `rust-toolchain.toml` for version)
- ripgrep (`rg`)
- fzf
- neovim
- gum (optional, for styled UI)
- direnv + nix (optional, see `.envrc` and `default.nix` in fsrs/)

### Environment Variables
- `MAIN_REPO`: Set to `$HOME/data/vn` in anki.sh (location of markdown notes)
- Database path is hardcoded to `/home/martins3/data/vn/docs/blog/anki.json` in main.rs

## Important Constraints

1. **Never run neovim inside neovim** - Scripts check for this and exit with error
2. **UUID must follow header immediately** - Scripts validate this on import
3. **No concurrent reviews** - JSON database has no locking mechanism
4. **Case-sensitive UUIDs** - Standard lowercase hex format only
5. **UTF-8 only** - No encoding detection or conversion

<script src="https://giscus.app/client.js"
        data-repo="martins3/martins3.github.io"
        data-repo-id="MDEwOlJlcG9zaXRvcnkyOTc4MjA0MDg="
        data-category="Show and tell"
        data-category-id="MDE4OkRpc2N1c3Npb25DYXRlZ29yeTMyMDMzNjY4"
        data-mapping="pathname"
        data-reactions-enabled="1"
        data-emit-metadata="0"
        data-theme="light"
        data-lang="zh-CN"
        crossorigin="anonymous"
        async>
</script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
