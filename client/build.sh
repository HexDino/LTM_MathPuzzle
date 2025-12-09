#!/bin/bash
# Build script for Math Puzzle Client on Linux/Mac

echo "================================"
echo "Math Puzzle Game - Qt Client Build"
echo "================================"
echo

# Check if qmake is available
if ! command -v qmake &> /dev/null; then
    echo "ERROR: qmake not found in PATH"
    echo "Please install Qt development tools"
    echo "Ubuntu/Debian: sudo apt-get install qt5-default qtbase5-dev"
    echo "Fedora: sudo dnf install qt5-qtbase-devel"
    echo "macOS: brew install qt"
    exit 1
fi

echo "Found qmake:"
qmake --version
echo

# Clean previous build
if [ -f Makefile ]; then
    echo "Cleaning previous build..."
    make clean 2>/dev/null
    rm -f Makefile* 2>/dev/null
    echo
fi

# Run qmake
echo "Running qmake..."
qmake MathPuzzleClient.pro
if [ $? -ne 0 ]; then
    echo "ERROR: qmake failed"
    exit 1
fi
echo

# Build
echo "Building with make..."
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2)
if [ $? -ne 0 ]; then
    echo
    echo "ERROR: Build failed"
    exit 1
fi

echo
echo "================================"
echo "Build successful!"
echo "================================"
echo
echo "To run the client:"
echo "  ./MathPuzzleClient"
echo


