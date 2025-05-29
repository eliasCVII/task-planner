# Task Planner Configuration Guide

## Overview

The Task Planner uses a configuration file called `plan.conf` to customize its behavior. This file should be placed in the same directory as the executable.

## Configuration File Format

The configuration file uses a simple key-value format:

```
# Comments start with # or ;
key: value
```

- **Comments**: Lines starting with `#` or `;` are ignored
- **Format**: `key: value` (colon separator)
- **Whitespace**: Leading and trailing spaces are automatically trimmed
- **Case Sensitive**: Setting names are case-sensitive

## Configuration Settings

### Core Settings

#### `data-dir`
**Purpose**: Specifies where task data files are stored
**Type**: String (file path)
**Default**: `data`
**Examples**:
```
data-dir: data                    # Relative path (default)
data-dir: /home/user/tasks        # Absolute path
data-dir: ~/my-tasks              # Home directory path
data-dir: /tmp/work-schedule      # Temporary directory
```

#### `default-day-length`
**Purpose**: Sets the default working day length in hours
**Type**: Decimal number
**Default**: `7.0`
**Range**: 0.1 to 24.0
**Examples**:
```
default-day-length: 7.0           # 7 hours (default)
default-day-length: 8.5           # 8 hours 30 minutes
default-day-length: 6.25          # 6 hours 15 minutes
default-day-length: 10.0          # 10 hours
```

#### `date-format`
**Purpose**: File naming date format (currently informational)
**Type**: String
**Default**: `YYYY-MM-DD`
**Examples**:
```
date-format: YYYY-MM-DD           # ISO format (default)
date-format: DD-MM-YYYY           # European format
date-format: MM-DD-YYYY           # US format
```
*Note: Currently only YYYY-MM-DD is implemented*

### UI and Behavior Settings

#### `auto-save`
**Purpose**: Automatically save data when exiting interactive mode
**Type**: Boolean
**Default**: `true`
**Valid Values**: `true`, `false`, `yes`, `no`, `1`, `0`, `on`, `off`
**Examples**:
```
auto-save: true                   # Enable auto-save (default)
auto-save: false                  # Disable auto-save
auto-save: yes                    # Alternative true value
auto-save: 0                      # Alternative false value
```

#### `show-warnings`
**Purpose**: Display calculation warnings and conflicts
**Type**: Boolean
**Default**: `true`
**Valid Values**: `true`, `false`, `yes`, `no`, `1`, `0`, `on`, `off`
**Examples**:
```
show-warnings: true               # Show warnings (default)
show-warnings: false              # Hide warnings
```

#### `default-start-time`
**Purpose**: Default start time for flexible tasks
**Type**: String (HH:MM format)
**Default**: `09:00`
**Format**: 24-hour format (HH:MM)
**Examples**:
```
default-start-time: 09:00         # 9:00 AM (default)
default-start-time: 08:30         # 8:30 AM
default-start-time: 07:00         # 7:00 AM
default-start-time: 10:15         # 10:15 AM
```

#### `time-format`
**Purpose**: Time display format preference
**Type**: String
**Default**: `24h`
**Valid Values**: `24h`, `12h`
**Examples**:
```
time-format: 24h                  # 24-hour format (default)
time-format: 12h                  # 12-hour format with AM/PM
```
*Note: Currently only 24h format is implemented*

### File Settings

#### `file-extension`
**Purpose**: File extension for data files
**Type**: String
**Default**: `.json`
**Examples**:
```
file-extension: .json             # JSON format (default)
file-extension: .txt              # Text format
file-extension: .data             # Custom extension
```
*Note: Content is always JSON regardless of extension*

#### `backup-enabled`
**Purpose**: Create backup files before saving
**Type**: Boolean
**Default**: `false`
**Valid Values**: `true`, `false`, `yes`, `no`, `1`, `0`, `on`, `off`
**Examples**:
```
backup-enabled: false             # No backups (default)
backup-enabled: true              # Create backups
```
*Note: Backup functionality is planned for future implementation*

#### `max-backup-files`
**Purpose**: Maximum number of backup files to keep
**Type**: Integer
**Default**: `5`
**Range**: 1 to 100
**Examples**:
```
max-backup-files: 5               # Keep 5 backups (default)
max-backup-files: 10              # Keep 10 backups
max-backup-files: 1               # Keep only 1 backup
```

### Display Settings

#### `table-width`
**Purpose**: Table width preference
**Type**: String
**Default**: `full`
**Valid Values**: `full`, `auto`
**Examples**:
```
table-width: full                 # Full terminal width (default)
table-width: auto                 # Auto-size to content
```

#### `status-messages`
**Purpose**: Show status and confirmation messages
**Type**: Boolean
**Default**: `true`
**Valid Values**: `true`, `false`, `yes`, `no`, `1`, `0`, `on`, `off`
**Examples**:
```
status-messages: true             # Show messages (default)
status-messages: false            # Hide messages
```

## Complete Example Configuration

```
# Task Planner Configuration File
# Format: key: value
# Lines starting with # or ; are comments

# Core Settings
data-dir: ~/work-tasks
default-day-length: 8.5
date-format: YYYY-MM-DD

# UI and Behavior
auto-save: true
show-warnings: true
default-start-time: 08:30
time-format: 24h

# File Settings
file-extension: .json
backup-enabled: false
max-backup-files: 5

# Display Settings
table-width: full
status-messages: true
```

## Configuration Loading

1. **File Location**: The program looks for `plan.conf` in the current working directory
2. **Missing File**: If not found, all default values are used
3. **Invalid Values**: Invalid settings use defaults with warnings
4. **Partial Config**: You can specify only the settings you want to change

## Session Persistence

The application automatically remembers the last opened file:

1. **Session File**: `.task_session` stores the path to the last opened data file
2. **Automatic Loading**: Interactive mode loads the last session automatically
3. **Priority Order**:
   - Command line date parameter (highest priority)
   - Last opened file (for interactive mode)
   - Today's date file (fallback)
4. **CLI Commands**: Don't affect session state (only interactive mode does)

## Error Handling

- **Missing config file**: Uses defaults, shows informational message
- **Invalid boolean values**: Uses default, shows warning
- **Invalid numeric values**: Uses default, shows warning
- **Malformed lines**: Skips line, shows warning
- **Invalid file paths**: May cause runtime errors when accessing files

## Tips

1. **Start Simple**: Begin with just the settings you need
2. **Test Changes**: Run `./plan list` to verify configuration is working
3. **Use Comments**: Document your custom settings
4. **Backup Config**: Keep a backup of your working configuration
5. **Check Paths**: Ensure data directory paths exist and are writable
