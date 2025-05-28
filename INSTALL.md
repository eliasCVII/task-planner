# Installation Guide

## Quick Install (Recommended)

```bash
# 1. Download source code (replace with actual repository URL)
git clone <repository-url>
cd ftxui-starter

# 2. Run automated setup
chmod +x setup.sh
./setup.sh

# 3. Start using the application
cd build
./proj
```

## Manual Installation

### Prerequisites

**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install build-essential cmake git
```

**macOS:**
```bash
xcode-select --install
brew install cmake
```

**Windows:**
- Install Visual Studio 2019+ with C++ support
- Install CMake from https://cmake.org/download/

### Build Steps

```bash
# 1. Create build directory
mkdir build
cd build

# 2. Configure with CMake
cmake ..

# 3. Build the application
make  # Linux/macOS
# or
cmake --build .  # Windows/Cross-platform

# 4. Test the build
./proj help
```

### Configuration (Optional)

```bash
# Create basic configuration
cat > plan.conf << EOF
data-dir: data
default-day-length: 8.0
auto-save: true
EOF

# Or copy and customize the example
cp ../plan.conf.example plan.conf
# Edit plan.conf with your preferred settings
```

## Verification

Test that everything works:

```bash
./proj help         # Should show help information
./proj list         # Should show default tasks
./proj              # Should launch interactive mode (press 'q' to quit)
```

## Next Steps

1. **Read the documentation:**
   - `README.md` - Complete user guide
   - `CONFIG.md` - Configuration options
   - `CONFIG_QUICK_REFERENCE.md` - Quick settings reference

2. **Customize your setup:**
   - Edit `plan.conf` for your preferences
   - Set up your preferred data directory
   - Adjust working hours and start times

3. **Start managing tasks:**
   - Use `./proj` for interactive mode
   - Use CLI commands for quick queries
   - Data is automatically saved to JSON files

## Troubleshooting

**Build fails:**
- Check that you have C++17 support
- Ensure CMake 3.11+ is installed
- Verify internet connection (for downloading dependencies)

**Application won't run:**
- Check that the executable has proper permissions
- Verify all dependencies were built correctly
- Try rebuilding: `rm -rf build && mkdir build && cd build && cmake .. && make`

**Configuration issues:**
- Ensure `plan.conf` is in the same directory as the executable
- Check file format and syntax (see CONFIG.md)
- Test without config first to verify basic functionality

## Directory Structure After Installation

```
ftxui-starter/
├── build/
│   ├── proj              # Main executable
│   ├── plan.conf         # Your configuration (optional)
│   └── data/             # Task data files (default location)
├── src/                  # Source code
├── README.md             # User guide
├── CONFIG.md             # Configuration documentation
└── setup.sh              # Automated setup script
```

## Uninstallation

To remove the application:

```bash
# Remove build directory
rm -rf build/

# Remove any data files (if you want to keep them, back them up first)
rm -rf data/  # or your custom data directory

# The source code directory can be deleted entirely if no longer needed
```
