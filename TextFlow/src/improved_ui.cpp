#include "improved_ui.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <cstring>

namespace TextFlow {

ImprovedUI::ImprovedUI() 
    : editor_(nullptr), running_(false), currentFile_("untitled.txt"),
      cursorX_(0), cursorY_(0), scrollX_(0), scrollY_(0),
      screenWidth_(80), screenHeight_(24), textStartY_(3),
      currentSearchIndex_(-1), searchActive_(false),
      terminalConfigured_(false), showMenu_(false), inCommandMode_(false) {
    
    // Get terminal size
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
        screenWidth_ = w.ws_col;
        screenHeight_ = w.ws_row;
        textStartY_ = 3; // Leave space for header and status
    }
    
    setupTerminal();
}

ImprovedUI::~ImprovedUI() {
    cleanup();
}

void ImprovedUI::setTextEditor(std::shared_ptr<TextEditor> editor) {
    editor_ = editor;
}

void ImprovedUI::setupTerminal() {
    if (terminalConfigured_) return;
    
    // Save original terminal settings
    tcgetattr(STDIN_FILENO, &originalTermios_);
    
    // Configure terminal for raw input
    struct termios raw = originalTermios_;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    
    // Set stdin to non-blocking
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    
    terminalConfigured_ = true;
}

void ImprovedUI::restoreTerminal() {
    if (!terminalConfigured_) return;
    
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &originalTermios_);
    terminalConfigured_ = false;
}

void ImprovedUI::run() {
    if (!editor_) {
        std::cerr << "Error: No text editor set!" << std::endl;
        return;
    }
    
    running_ = true;
    clearScreen();
    
    while (running_) {
        displayHeader();
        displayText();
        displayStatusBar();
        
        if (showMenu_) {
            displayMenu();
        }
        
        handleInput();
    }
    
    cleanup();
}

void ImprovedUI::cleanup() {
    restoreTerminal();
    if (editor_) {
        editor_->saveDocument();
    }
}

void ImprovedUI::clearScreen() {
    system("clear");
}

void ImprovedUI::displayHeader() {
    std::cout << "\033[1;34m" << "TextFlow - Advanced Text Editor" << "\033[0m" << std::endl;
    std::cout << "File: " << currentFile_ << " | ";
    std::cout << "Lines: " << editor_->getLineCount() << " | ";
    std::cout << "Modified: " << (editor_->isModified() ? "Yes" : "No") << std::endl;
    std::cout << std::string(screenWidth_, '-') << std::endl;
}

void ImprovedUI::displayText() {
    std::string text = editor_->getText();
    std::istringstream iss(text);
    std::string line;
    int lineNum = 1;
    int displayLines = screenHeight_ - textStartY_ - 2; // Leave space for status bar
    
    // Move cursor to text area
    std::cout << "\033[" << textStartY_ << ";1H";
    
    while (std::getline(iss, line) && lineNum <= displayLines) {
        // Clear the line first
        std::cout << "\033[K";
        
        // Display line number
        std::cout << "\033[90m" << std::setw(3) << lineNum << "|\033[0m ";
        
        // Display line content
        if (line.length() > screenWidth_ - 10) {
            line = line.substr(0, screenWidth_ - 13) + "...";
        }
        std::cout << line;
        
        // Move to next line
        std::cout << "\033[0K" << std::endl;
        lineNum++;
    }
    
    // Clear remaining lines
    for (int i = lineNum; i <= displayLines; i++) {
        std::cout << "\033[K" << std::endl;
    }
}

void ImprovedUI::displayStatusBar() {
    // Move to bottom of screen
    std::cout << "\033[" << screenHeight_ - 1 << ";1H";
    std::cout << "\033[1;47;30m"; // White background, black text
    
    std::string status = "Line: " + std::to_string(editor_->getCurrentLine()) + 
                        " Col: " + std::to_string(editor_->getCurrentColumn()) +
                        " | Press 'h' for help, ':' for commands";
    
    if (!statusMessage_.empty()) {
        status += " | " + statusMessage_;
        statusMessage_.clear();
    }
    
    if (!errorMessage_.empty()) {
        status += " | ERROR: " + errorMessage_;
        errorMessage_.clear();
    }
    
    // Pad with spaces to fill the line
    status.resize(screenWidth_, ' ');
    std::cout << status << "\033[0m" << std::endl;
}

void ImprovedUI::displayMenu() {
    std::cout << "\n\033[1;32m--- Commands ---\033[0m" << std::endl;
    std::cout << "File: [o]pen [s]ave [n]ew [w]rite [q]uit" << std::endl;
    std::cout << "Edit: [i]nsert [d]elete [u]ndo [r]edo [a]ppend" << std::endl;
    std::cout << "Search: [/]find [?]replace [n]ext [N]previous" << std::endl;
    std::cout << "Navigation: [g]oto line [h]ome [e]nd" << std::endl;
    std::cout << "Other: [h]elp [c]lear [t]est [x]it" << std::endl;
    std::cout << "\nPress any key to continue...";
}

void ImprovedUI::handleInput() {
    char c;
    if (read(STDIN_FILENO, &c, 1) > 0) {
        if (inCommandMode_) {
            handleCommand();
        } else {
            handleKeyPress();
        }
    }
}

void ImprovedUI::handleKeyPress() {
    char c;
    if (read(STDIN_FILENO, &c, 1) <= 0) return;
    
    switch (c) {
        case 'q':
            running_ = false;
            break;
            
        case 'h':
            showMenu_ = !showMenu_;
            break;
            
        case ':':
            inCommandMode_ = true;
            commandBuffer_.clear();
            showMessage("Enter command:");
            break;
            
        case 'o':
            openFile();
            break;
            
        case 's':
            saveFile();
            break;
            
        case 'n':
            newFile();
            break;
            
        case 'w':
            saveAsFile();
            break;
            
        case 'i':
            insertChar(' ');
            break;
            
        case 'd':
            deleteChar();
            break;
            
        case 'u':
            editor_->undo();
            showMessage("Undo performed");
            break;
            
        case 'r':
            editor_->redo();
            showMessage("Redo performed");
            break;
            
        case '/':
            searchText();
            break;
            
        case '?':
            replaceText();
            break;
            
        case 'g':
            moveToLine(1);
            break;
            
        case 't':
            testEditor();
            break;
            
        case 'c':
            clearScreen();
            break;
            
        case 27: // Escape key
            if (showMenu_) {
                showMenu_ = false;
            }
            break;
            
        default:
            if (c >= 32 && c <= 126) { // Printable characters
                insertChar(c);
            }
            break;
    }
}

void ImprovedUI::handleCommand() {
    char c;
    if (read(STDIN_FILENO, &c, 1) <= 0) return;
    
    if (c == '\n' || c == '\r') {
        // Execute command
        if (commandBuffer_ == "q" || commandBuffer_ == "quit") {
            running_ = false;
        } else if (commandBuffer_ == "w" || commandBuffer_ == "write") {
            saveFile();
        } else if (commandBuffer_ == "wq") {
            saveFile();
            running_ = false;
        } else if (commandBuffer_.substr(0, 2) == "w ") {
            currentFile_ = commandBuffer_.substr(2);
            saveFile();
        } else {
            showError("Unknown command: " + commandBuffer_);
        }
        
        inCommandMode_ = false;
        commandBuffer_.clear();
    } else if (c == 27) { // Escape
        inCommandMode_ = false;
        commandBuffer_.clear();
    } else if (c == 127 || c == 8) { // Backspace
        if (!commandBuffer_.empty()) {
            commandBuffer_.pop_back();
        }
    } else if (c >= 32 && c <= 126) {
        commandBuffer_ += c;
    }
}

void ImprovedUI::insertChar(char c) {
    std::string text(1, c);
    editor_->insertText(text);
    showMessage("Character inserted");
}

void ImprovedUI::deleteChar() {
    editor_->deleteText(1);
    showMessage("Character deleted");
}

void ImprovedUI::moveCursor(int dx, int dy) {
    // This would need to be implemented with proper cursor tracking
    // For now, we'll just show a message
    showMessage("Cursor movement not fully implemented");
}

void ImprovedUI::moveToLine(int line) {
    editor_->moveCursorToLine(line - 1);
    showMessage("Moved to line " + std::to_string(line));
}

void ImprovedUI::searchText() {
    std::string term = getInput("Search for: ");
    if (!term.empty()) {
        searchTerm_ = term;
        searchResults_ = editor_->findText(term);
        currentSearchIndex_ = 0;
        searchActive_ = true;
        
        if (searchResults_.empty()) {
            showMessage("No matches found for: " + term);
        } else {
            showMessage("Found " + std::to_string(searchResults_.size()) + " matches");
        }
    }
}

void ImprovedUI::replaceText() {
    std::string find = getInput("Find: ");
    if (!find.empty()) {
        std::string replace = getInput("Replace with: ");
        editor_->replaceText(find, replace, true);
        showMessage("Replace completed");
    }
}

void ImprovedUI::openFile() {
    std::string filename = getInput("Open file: ");
    if (!filename.empty()) {
        if (editor_->openDocument(filename)) {
            currentFile_ = filename;
            showMessage("Opened: " + filename);
        } else {
            showError("Failed to open: " + filename);
        }
    }
}

void ImprovedUI::saveFile() {
    if (editor_->saveDocument()) {
        showMessage("File saved: " + currentFile_);
    } else {
        showError("Failed to save file");
    }
}

void ImprovedUI::saveAsFile() {
    std::string filename = getInput("Save as: ");
    if (!filename.empty()) {
        if (editor_->saveAsDocument(filename)) {
            currentFile_ = filename;
            showMessage("Saved as: " + filename);
        } else {
            showError("Failed to save as: " + filename);
        }
    }
}

void ImprovedUI::newFile() {
    editor_->newDocument();
    currentFile_ = "untitled.txt";
    showMessage("New document created");
}

void ImprovedUI::showMessage(const std::string& msg) {
    statusMessage_ = msg;
}

void ImprovedUI::showError(const std::string& error) {
    errorMessage_ = error;
}

std::string ImprovedUI::getInput(const std::string& prompt) {
    // This is a simplified input function
    // In a real implementation, this would handle the input properly
    std::cout << "\n" << prompt;
    std::string input;
    std::getline(std::cin, input);
    return input;
}

void ImprovedUI::testEditor() {
    showMessage("Testing editor features...");
    
    // Test basic operations
    editor_->insertText("Hello, World!\nThis is a test line.\nLine 3: More text here.");
    
    // Test search
    auto results = editor_->findText("test");
    showMessage("Search test: Found " + std::to_string(results.size()) + " matches");
    
    // Test undo/redo
    editor_->undo();
    showMessage("Undo test completed");
    
    editor_->redo();
    showMessage("Redo test completed");
}

} // namespace TextFlow
