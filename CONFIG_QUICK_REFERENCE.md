# Task Manager Configuration Quick Reference

## File Location
- **Filename**: `plan.conf`
- **Location**: Same directory as executable
- **Format**: `key: value` (one per line)
- **Comments**: Lines starting with `#` or `;`

## Essential Settings

| Setting | Type | Default | Description |
|---------|------|---------|-------------|
| `data-dir` | Path | `data` | Where to store task files |
| `default-day-length` | Number | `7.0` | Working hours per day |
| `auto-save` | Boolean | `true` | Save automatically on exit |

## All Settings Reference

### Core Settings
```
data-dir: ~/my-tasks              # Custom data directory
default-day-length: 8.5           # 8.5 hours working day
date-format: YYYY-MM-DD           # Date format (fixed)
```

### Behavior Settings
```
auto-save: true                   # Auto-save on exit
show-warnings: true               # Show calculation warnings
default-start-time: 09:00         # Default start time (HH:MM)
time-format: 24h                  # Time display format
```

### File Settings
```
file-extension: .json             # File extension
backup-enabled: false             # Create backups (planned)
max-backup-files: 5               # Max backup count (planned)
```

### Display Settings
```
table-width: full                 # Table width (full/auto)
status-messages: true             # Show status messages
```

## Boolean Values
Any of these work for true/false settings:
- **True**: `true`, `yes`, `1`, `on`
- **False**: `false`, `no`, `0`, `off`

## Common Examples

### Basic Home Setup
```
data-dir: ~/tasks
default-day-length: 8.0
auto-save: true
```

### Team/Shared Setup
```
data-dir: /shared/team-schedule
default-day-length: 7.5
status-messages: false
```

### Part-time Schedule
```
data-dir: ~/part-time
default-day-length: 4.0
default-start-time: 13:00
```

## Quick Start
1. Copy `plan.conf.example` to `plan.conf`
2. Edit the settings you want to change
3. Run `./proj list` to test
4. Use `./proj` for interactive mode

## Troubleshooting
- **Config not loading**: Check file is named `plan.conf` in correct directory
- **Invalid values**: Check CONFIG.md for valid value formats
- **Path errors**: Ensure data directory exists and is writable
- **No effect**: Restart application after config changes
