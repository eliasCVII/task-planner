# FTXUI Task Manager

A terminal-based interactive task management application with vim-like navigation, data persistence, and flexible configuration.

## Features

- **Interactive UI** - Full-screen terminal interface with vim-like navigation
- **Task Management** - Create, edit, move, and delete tasks with flexible/fixed scheduling
- **Data Persistence** - Automatic saving with JSON format for easy editing
- **Session Memory** - Automatically reopens the last used file on startup
- **Configuration** - Customizable settings via `plan.conf` file
- **CLI Commands** - Query tasks from command line (`now`, `next`, `list`)
- **Date Support** - Daily task files with historical data access

## Quick Start

### 1. Prerequisites

**Required:**
- C++17 compatible compiler (GCC 8+, Clang 7+, MSVC 2019+)
- CMake 3.11 or higher
- Git (for downloading dependencies)

**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install build-essential cmake git
```

**macOS:**
```bash
# Install Xcode Command Line Tools
xcode-select --install
# Install CMake (via Homebrew)
brew install cmake
```

**Windows:**
- Install Visual Studio 2019+ with C++ support
- Install CMake from https://cmake.org/download/

### 2. Download and Build

```bash
# Clone or download the source code
git clone <repository-url>  # or download and extract ZIP
cd ftxui-starter

# Create build directory
mkdir build
cd build

# Configure and build
cmake ..
make  # or 'cmake --build .' on Windows

# The executable will be created as 'proj'
```

### 3. First Run

```bash
# Test the application
./proj help

# Run interactive mode
./proj

# Try CLI commands
./proj list
./proj now
./proj next
```

## Configuration

### Basic Setup

1. **Create configuration file** (optional):
```bash
# Copy the example configuration
cp ../plan.conf.example plan.conf

# Edit with your preferred settings
nano plan.conf  # or vim, code, etc.
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

**Quick examples:**

```bash
# Home office setup
cat > plan.conf << EOF
data-dir: ~/work-tasks
default-day-length: 8.0
default-start-time: 09:00
auto-save: true
EOF

# Part-time schedule
cat > plan.conf << EOF
data-dir: ~/part-time-work
default-day-length: 4.0
default-start-time: 13:00
auto-save: true
EOF
```

## Usage

### Interactive Mode

```bash
./proj  # Launch interactive interface
```

**Navigation:**
- `hjkl` or arrow keys - Navigate
- `Enter` - Edit cell or toggle boolean values
- `Tab` - Move to next field during editing
- `i/o` - Insert new task before/after current
- `v` - Visual mode for moving tasks
- `dd` or `D` - Delete task
- `q` - Quit and save
- `Esc` - Cancel editing/visual mode

### Command Line Interface

```bash
./proj now                    # Show current active task
./proj next                   # Show next upcoming task
./proj list                   # Show all tasks for today
./proj list 2024-01-15       # Show tasks for specific date
./proj help                   # Show help information
```

### Data Files

Tasks are automatically saved to JSON files:
- **Location**: Configured via `data-dir` (default: `data/`)
- **Format**: `tasks_YYYY-MM-DD.json`
- **Content**: Human-readable JSON that can be edited manually

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
ftxui-starter/
├── src/                      # Source code
├── build/                    # Build directory (created by you)
│   ├── proj                  # Executable
│   ├── plan.conf             # Your configuration (optional)
│   └── data/                 # Task data files (default location)
├── plan.conf.example         # Example configuration
├── CONFIG.md                 # Configuration documentation
└── README.md                 # This file
```

## Troubleshooting

### Build Issues

**CMake not found:**
```bash
# Ubuntu/Debian
sudo apt install cmake

# macOS
brew install cmake
```

**Compiler errors:**
- Ensure you have C++17 support
- Try updating your compiler
- Check CMake version (3.11+ required)

**Dependencies not downloading:**
- Check internet connection
- Ensure Git is installed
- Try deleting `build/` and rebuilding

### Runtime Issues

**Config not loading:**
- Ensure `plan.conf` is in the same directory as the executable
- Check file permissions
- Verify file format (see CONFIG.md)

**Data not saving:**
- Check data directory permissions
- Verify `data-dir` path in config
- Ensure disk space available

**Tasks not displaying correctly:**
- Check `default-day-length` setting
- Verify task time formats (HH:MM)
- See CONFIG.md for valid values

## Development

### Project Structure
- `src/main.cpp` - Main application and UI
- `src/TaskManager.cpp/h` - Task management logic
- `src/Act.cpp/h` - Individual task (Act) implementation
- `src/Config.cpp/h` - Configuration system
- `CMakeLists.txt` - Build configuration

### Dependencies
- **FTXUI** - Terminal UI library (auto-downloaded)
- **nlohmann/json** - JSON parsing (auto-downloaded)

### Building from Source
The build system automatically downloads and builds all dependencies.

## License

[Add your license information here]

## Contributing

[Add contribution guidelines here]
