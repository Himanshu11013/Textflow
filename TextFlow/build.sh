#!/bin/bash

# TextFlow Build Script
# This script builds the TextFlow text editor and sets up the Python NLP service

set -e  # Exit on any error

echo "🚀 Building TextFlow - Advanced Text Editor"
echo "=========================================="

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ]; then
    print_error "CMakeLists.txt not found. Please run this script from the TextFlow root directory."
    exit 1
fi

# Check dependencies
print_status "Checking dependencies..."

# Check for required tools
command -v cmake >/dev/null 2>&1 || { print_error "cmake is required but not installed. Please install cmake."; exit 1; }
command -v make >/dev/null 2>&1 || { print_error "make is required but not installed. Please install make."; exit 1; }
command -v python3 >/dev/null 2>&1 || { print_error "python3 is required but not installed. Please install python3."; exit 1; }
command -v pip3 >/dev/null 2>&1 || { print_error "pip3 is required but not installed. Please install pip3."; exit 1; }

print_success "All required tools found"

# Check for required libraries
print_status "Checking for required libraries..."

# Check ncurses
if ! pkg-config --exists ncurses; then
    print_error "ncurses development libraries not found. Please install libncurses5-dev (Ubuntu) or ncurses (macOS)"
    exit 1
fi

# Check OpenSSL
if ! pkg-config --exists openssl; then
    print_error "OpenSSL development libraries not found. Please install libssl-dev (Ubuntu) or openssl (macOS)"
    exit 1
fi

# Check libcurl
if ! pkg-config --exists libcurl; then
    print_error "libcurl development libraries not found. Please install libcurl4-openssl-dev (Ubuntu) or curl (macOS)"
    exit 1
fi

print_success "All required libraries found"

# Create build directory
print_status "Creating build directory..."
mkdir -p build
cd build

# Configure with CMake
print_status "Configuring with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Build the project
print_status "Building TextFlow..."
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

print_success "TextFlow built successfully"

# Go back to root directory
cd ..

# Set up Python NLP service
print_status "Setting up Python NLP service..."

# Check if virtual environment exists
if [ ! -d "python_nlp_service/venv" ]; then
    print_status "Creating Python virtual environment..."
    cd python_nlp_service
    python3 -m venv venv
    cd ..
fi

# Activate virtual environment and install dependencies
print_status "Installing Python dependencies..."
cd python_nlp_service
source venv/bin/activate
pip install --upgrade pip
pip install -r requirements.txt

# Download spaCy model
print_status "Downloading spaCy English model..."
python -m spacy download en_core_web_sm

print_success "Python NLP service setup complete"

# Go back to root directory
cd ..

# Create necessary directories
print_status "Creating configuration directories..."
mkdir -p ~/.textflow
mkdir -p ~/.textflow/logs
mkdir -p ~/.textflow/themes
mkdir -p ~/.textflow/backups

# Create default configuration
print_status "Creating default configuration..."
cat > ~/.textflow/config.json << EOF
{
  "theme": "dark",
  "line_numbers": true,
  "word_wrap": true,
  "syntax_highlighting": true,
  "auto_complete": true,
  "nlp_features": true,
  "auto_save": true,
  "auto_save_interval": 30,
  "undo_history_size": 50,
  "tab_size": 4,
  "font_size": 12,
  "nlp_service_url": "http://localhost:8000"
}
EOF

# Create desktop entry (Linux)
if command -v xdg-desktop-menu >/dev/null 2>&1; then
    print_status "Creating desktop entry..."
    cat > ~/.local/share/applications/textflow.desktop << EOF
[Desktop Entry]
Version=1.0
Type=Application
Name=TextFlow
Comment=Advanced Text Editor
Exec=$(pwd)/build/textflow %F
Icon=$(pwd)/assets/textflow-icon.png
Terminal=false
Categories=TextEditor;Development;
MimeType=text/plain;text/x-c;text/x-c++;text/x-python;text/x-java;text/x-html;text/x-css;text/x-javascript;
EOF
    chmod +x ~/.local/share/applications/textflow.desktop
    print_success "Desktop entry created"
fi

# Run tests
print_status "Running tests..."

# C++ tests
if [ -f "build/test_runner" ]; then
    print_status "Running C++ tests..."
    cd build
    ./test_runner
    cd ..
    print_success "C++ tests passed"
else
    print_warning "C++ test runner not found, skipping C++ tests"
fi

# Python tests
print_status "Running Python tests..."
cd python_nlp_service
source venv/bin/activate
if command -v pytest >/dev/null 2>&1; then
    python -m pytest tests/ -v
    print_success "Python tests passed"
else
    print_warning "pytest not found, skipping Python tests"
fi
cd ..

# Create launch script
print_status "Creating launch script..."
cat > textflow.sh << 'EOF'
#!/bin/bash

# TextFlow Launch Script
# This script starts the NLP service and launches the text editor

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Function to check if a port is in use
check_port() {
    local port=$1
    if lsof -Pi :$port -sTCP:LISTEN -t >/dev/null 2>&1; then
        return 0
    else
        return 1
    fi
}

# Function to start NLP service
start_nlp_service() {
    echo "Starting NLP service..."
    cd "$SCRIPT_DIR/python_nlp_service"
    source venv/bin/activate
    python main.py &
    NLP_PID=$!
    echo "NLP service started with PID: $NLP_PID"
    
    # Wait for service to be ready
    echo "Waiting for NLP service to be ready..."
    for i in {1..30}; do
        if curl -s http://localhost:8000/health >/dev/null 2>&1; then
            echo "NLP service is ready!"
            return 0
        fi
        sleep 1
    done
    
    echo "Warning: NLP service may not be ready"
    return 1
}

# Function to stop NLP service
stop_nlp_service() {
    if [ ! -z "$NLP_PID" ]; then
        echo "Stopping NLP service (PID: $NLP_PID)..."
        kill $NLP_PID 2>/dev/null || true
    fi
}

# Set up signal handlers
trap stop_nlp_service EXIT

# Check if NLP service is already running
if check_port 8000; then
    echo "NLP service is already running on port 8000"
else
    start_nlp_service
fi

# Launch the text editor
echo "Launching TextFlow..."
cd "$SCRIPT_DIR"
./build/textflow "$@"

# Clean up
stop_nlp_service
EOF

chmod +x textflow.sh

print_success "Launch script created: textflow.sh"

# Create uninstall script
print_status "Creating uninstall script..."
cat > uninstall.sh << 'EOF'
#!/bin/bash

echo "Uninstalling TextFlow..."

# Remove build directory
if [ -d "build" ]; then
    echo "Removing build directory..."
    rm -rf build
fi

# Remove Python virtual environment
if [ -d "python_nlp_service/venv" ]; then
    echo "Removing Python virtual environment..."
    rm -rf python_nlp_service/venv
fi

# Remove configuration directory
if [ -d ~/.textflow ]; then
    echo "Removing configuration directory..."
    rm -rf ~/.textflow
fi

# Remove desktop entry
if [ -f ~/.local/share/applications/textflow.desktop ]; then
    echo "Removing desktop entry..."
    rm ~/.local/share/applications/textflow.desktop
fi

# Remove launch script
if [ -f "textflow.sh" ]; then
    echo "Removing launch script..."
    rm textflow.sh
fi

echo "TextFlow uninstalled successfully"
EOF

chmod +x uninstall.sh

print_success "Uninstall script created: uninstall.sh"

# Final summary
echo ""
echo "🎉 TextFlow build completed successfully!"
echo "=========================================="
echo ""
echo "To run TextFlow:"
echo "  ./textflow.sh [filename]"
echo ""
echo "To run without NLP features:"
echo "  ./build/textflow [filename]"
echo ""
echo "To uninstall:"
echo "  ./uninstall.sh"
echo ""
echo "Configuration files are located in: ~/.textflow/"
echo "Logs are located in: ~/.textflow/logs/"
echo ""
echo "For more information, see README.md"
echo ""
