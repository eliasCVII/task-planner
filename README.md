# Task Planner

A terminal-based interactive task planner with vim-like navigation, data persistence, and flexible configuration.

## Features

- **Interactive UI** - Full-screen terminal interface with vim-like navigation
- **Task Planning** - Create, edit, move, and delete tasks with flexible/fixed scheduling
- **Data Persistence** - Automatic saving with JSON format for easy editing
- **Session Memory** - Automatically reopens the last used file on startup
- **Configuration** - Customizable settings via `plan.conf` file
- **CLI Commands** - Query tasks from command line (`now`, `next`, `list`)
- **Date Support** - Daily task files with historical data access

## Prerequisites

- C++17 compatible compiler
- CMake 3.11 or higher
- Git (for fetching dependencies)
- `fzf` (recommended for best file browser experience)
- `fd` or `find` (alternative file discovery tools)
- If none are available, falls back to simple numbered selection

## Quick Start

### 1. Download and Build

```bash
# Clone or download the source code
git clone https://github.com/eliasCVII/task-planner.git  # or download and extract ZIP
cd task-planner

# Create build directory
mkdir build
cd build

# Configure and build
cmake ..
make  # or 'cmake --build .' on Windows

# The executable will be created as 'plan'
```

### 2. First Run

```bash
# Test the application
./plan help

# Run interactive mode
./plan

# Try CLI commands
./plan list
./plan now
./plan next
```

## Configuration

### Basic Setup

1. **Create configuration file** (optional):
```bash
# Copy the example configuration
cp ../plan.conf.example ~/.config/plan/plan.conf

# Edit with your preferred settings
nano ~/.config/plan/plan.conf  # or vim, code, etc.
```

2. **Essential settings**:
```
# Basic configuration
data-dir: ~/my-tasks           # Where to store task files
default-day-length: 8.0        # Working hours per day
auto-save: true                # Save automatically on exit
```

### Configuration Options

See [CONFIG.md](CONFIG.md) for complete configuration documentation.

## Usage

### Interactive Mode

```bash
./plan  # Launch interactive interface
```

**Navigation:**
- `hjkl` or arrow keys - Navigate
- `Enter` - Edit cell or toggle boolean values
- `Tab` - Move to next field during editing
- `i/o` - Insert new task before/after current
- `v` - Visual mode for moving tasks
- `f` - File browser (select any JSON task file)
- `dd` or `D` - Delete task
- `q` - Quit and save
- `Esc` - Cancel editing/visual mode

### Command Line Interface

```bash
./plan now                    # Show current active task
./plan next                   # Show next upcoming task
./plan list                   # Show all tasks for today
./plan list 2024-01-15        # Show tasks for specific date
./plan help                   # Show help information
```

### Data Files

Tasks are automatically saved to JSON files:
- **Location**: Configured via `data-dir` (default: `data/`)
- **Format**: Any `.json` filename (not limited to date-based naming)
- **Content**: Human-readable JSON that can be edited manually

#### File Browser

The interactive file browser (`f` key) allows you to:
- Browse all JSON files in your data directory
- Select files with any naming convention (not just date-based)
- Automatic tool detection with fallback options:
  - **Best**: `fzf` with fuzzy search and file preview
  - **Good**: `fd` for fast file discovery (with or without `fzf`)
  - **Basic**: `find` for standard file discovery
  - **Fallback**: Simple numbered selection if no tools available
- Switch between different task files seamlessly

Example data file:
```json
{
  "date": "2024-01-15",
  "dayLength": 480,
  "tasks": [
    {
      "name": "Morning Meeting",
      "startTime": "09:00",
      "length": 60,
      "rigid": false,
      "fixed": true
    }
  ]
}
```

## Task Types

- **Fixed Tasks** - Have specific start times that don't change
- **Flexible Tasks** - Start times calculated automatically
- **Rigid Tasks** - Have fixed durations
- **Flexible Tasks** - Durations scale based on available time

## Directory Structure

```
task-planner/
├── src/                      # Source code
├── build/                    # Build directory (created by you)
│   ├── plan                  # Executable
│   ├── plan.conf             # Your configuration (optional)
│   └── data/                 # Task data files (default location)
├── plan.conf.example         # Example configuration
├── CONFIG.md                 # Configuration documentation
└── README.md                 # This file
```
