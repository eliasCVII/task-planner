# Getting Started with FTXUI Task Manager

## For New Users (Just Downloaded Source Code)

### Option 1: Automated Setup (Recommended)

```bash
# 1. Navigate to the downloaded source code directory
cd ftxui-starter

# 2. Run the automated setup script
chmod +x setup.sh
./setup.sh

# 3. Follow the interactive prompts
# The script will:
# - Check prerequisites
# - Create build directory
# - Download dependencies and build
# - Set up basic configuration
# - Test the installation

# 4. Start using the application
cd build
./proj
```

### Option 2: Manual Setup

```bash
# 1. Navigate to source directory
cd ftxui-starter

# 2. Create and enter build directory
mkdir build
cd build

# 3. Configure and build
cmake ..
make

# 4. Test the build
./proj help

# 5. Create basic configuration (optional)
cat > plan.conf << EOF
data-dir: data
default-day-length: 8.0
auto-save: true
EOF

# 6. Start using
./proj
```

## What You Get After Setup

### Directory Structure
```
ftxui-starter/
├── build/                    # Build directory (you created this)
│   ├── proj                  # ← Main executable
│   ├── plan.conf             # ← Your configuration (optional)
│   ├── data/                 # ← Task data files (created automatically)
│   └── [build files...]      # CMake/build artifacts
├── src/                      # Source code
├── README.md                 # Complete user guide
├── CONFIG.md                 # Configuration documentation
├── INSTALL.md                # Installation guide
├── setup.sh                  # Automated setup script
└── plan.conf.example         # Example configuration
```

### First Run Experience

1. **Test basic functionality:**
```bash
cd build
./proj help         # Shows help and confirms it works
./proj list         # Shows sample tasks
```

2. **Try interactive mode:**
```bash
./proj              # Launch full-screen interface
# Press 'q' to quit and save
```

3. **Check data persistence:**
```bash
ls data/            # Should show tasks_YYYY-MM-DD.json
cat data/tasks_*.json  # View the saved data
```

## Configuration Quick Start

### Basic Configuration
Create `build/plan.conf` with your preferences:

```bash
# Essential settings
data-dir: ~/my-tasks           # Where to store task files
default-day-length: 8.0        # Your working hours per day
auto-save: true                # Save automatically on exit
```

### Common Configurations

**Home Office:**
```
data-dir: ~/work-tasks
default-day-length: 8.0
default-start-time: 09:00
auto-save: true
status-messages: true
```

**Part-time Work:**
```
data-dir: ~/part-time
default-day-length: 4.0
default-start-time: 13:00
auto-save: true
```

**Team/Shared Setup:**
```
data-dir: /shared/team-schedule
default-day-length: 7.5
status-messages: false
auto-save: true
```

## Usage Patterns

### Daily Workflow
```bash
# Morning: Check today's schedule
./proj list

# Check current task
./proj now

# Interactive planning/editing
./proj

# Quick status check
./proj next
```

### CLI Integration
```bash
# Add to shell aliases
alias task-now='./proj now'
alias task-next='./proj next'
alias task-list='./proj list'
alias task-edit='./proj'

# Use in scripts
current_task=$(./proj now)
echo "Currently working on: $current_task"
```

## Troubleshooting First-Time Issues

### Build Problems

**"cmake not found":**
```bash
# Ubuntu/Debian
sudo apt install cmake

# macOS
brew install cmake
```

**"No C++ compiler":**
```bash
# Ubuntu/Debian
sudo apt install build-essential

# macOS
xcode-select --install
```

**"Build fails with dependency errors":**
```bash
# Clean rebuild
rm -rf build
mkdir build
cd build
cmake ..
make
```

### Runtime Problems

**"Config file not found" (but you created one):**
- Ensure `plan.conf` is in the same directory as the `proj` executable
- Check you're running from the `build/` directory

**"Permission denied" when saving:**
- Check data directory permissions
- Try using a different data directory (like `~/tasks`)

**"Application crashes on startup":**
- Test without config: `mv plan.conf plan.conf.backup && ./proj list`
- Check config file syntax (see CONFIG.md)

## Next Steps

1. **Read the documentation:**
   - `README.md` - Complete feature overview
   - `CONFIG.md` - All configuration options
   - `CONFIG_QUICK_REFERENCE.md` - Quick settings lookup

2. **Customize your workflow:**
   - Set up your preferred data directory
   - Adjust working hours and start times
   - Configure auto-save and display preferences

3. **Learn the interface:**
   - Practice vim-like navigation (`hjkl`)
   - Try editing tasks (Enter key)
   - Experiment with visual mode (`v` key)
   - Learn insertion commands (`i`, `o`)

4. **Integrate with your system:**
   - Add shell aliases for quick access
   - Set up data directory in a synced location
   - Consider adding to your daily routine

## Support

If you encounter issues:

1. **Check the documentation** - Most questions are answered in the docs
2. **Verify your setup** - Run `./proj help` to ensure basic functionality
3. **Test without config** - Remove `plan.conf` temporarily to test defaults
4. **Check file permissions** - Ensure you can read/write in the data directory

The application is designed to work out-of-the-box with sensible defaults, so if you're having issues, it's likely a configuration or permissions problem that can be easily resolved.

Happy task managing! 🚀
