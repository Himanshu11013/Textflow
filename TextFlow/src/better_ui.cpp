#include "better_ui.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <unistd.h>
#include <sys/ioctl.h>

namespace TextFlow {

BetterUI::BetterUI() 
    : editor_(nullptr), running_(false), currentFile_("untitled.txt"),
      screenWidth_(80), screenHeight_(24), showMenu_(false) {
    
    // Get terminal size
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
        screenWidth_ = w.ws_col;
        screenHeight_ = w.ws_row;
    }
}

BetterUI::~BetterUI() {
    cleanup();
}

void BetterUI::setTextEditor(std::shared_ptr<TextEditor> editor) {
    editor_ = editor;
}

void BetterUI::run() {
    if (!editor_) {
        std::cerr << "Error: No text editor set!" << std::endl;
        return;
    }
    
    running_ = true;
    
    while (running_) {
        clearScreen();
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

void BetterUI::cleanup() {
    if (editor_) {
        editor_->saveDocument();
    }
}

void BetterUI::clearScreen() {
    system("clear");
}

void BetterUI::displayHeader() {
    std::cout << "\033[1;34m" << "TextFlow - Advanced Text Editor" << "\033[0m" << std::endl;
    std::cout << "File: " << currentFile_ << " | ";
    std::cout << "Lines: " << editor_->getLineCount() << " | ";
    std::cout << "Modified: " << (editor_->isModified() ? "Yes" : "No") << std::endl;
    std::cout << std::string(screenWidth_, '-') << std::endl;
}

void BetterUI::displayText() {
    std::string text = editor_->getText();
    std::istringstream iss(text);
    std::string line;
    int lineNum = 1;
    int maxLines = screenHeight_ - 6; // Leave space for header and status
    
    std::cout << "\n--- Text Content ---" << std::endl;
    
    if (text.empty()) {
        std::cout << "  1| (empty document)" << std::endl;
        return;
    }
    
    while (std::getline(iss, line) && lineNum <= maxLines) {
        std::cout << std::setw(3) << lineNum << "| ";
        
        // Truncate long lines
        if (line.length() > screenWidth_ - 10) {
            line = line.substr(0, screenWidth_ - 13) + "...";
        }
        std::cout << line << std::endl;
        lineNum++;
    }
    
    if (lineNum > maxLines) {
        std::cout << "     ... (more lines available)" << std::endl;
    }
}

void BetterUI::displayStatusBar() {
    std::cout << "\n--- Status ---" << std::endl;
    std::cout << "Current Line: " << editor_->getCurrentLine() << " | ";
    std::cout << "Current Column: " << editor_->getCurrentColumn() << " | ";
    std::cout << "NLP: OFF (Simplified)" << std::endl;
    
    if (!statusMessage_.empty()) {
        std::cout << "\033[1;32m" << "INFO: " << statusMessage_ << "\033[0m" << std::endl;
        statusMessage_.clear();
    }
    
    if (!errorMessage_.empty()) {
        std::cout << "\033[1;31m" << "ERROR: " << errorMessage_ << "\033[0m" << std::endl;
        errorMessage_.clear();
    }
}

void BetterUI::displayMenu() {
    std::cout << "\n\033[1;32m--- Commands ---\033[0m" << std::endl;
    std::cout << "File: [o]pen [s]ave [n]ew [w]rite [q]uit" << std::endl;
    std::cout << "Edit: [i]nsert [d]elete [u]ndo [r]edo [a]ppend" << std::endl;
    std::cout << "Search: [f]ind [F]ind&Replace" << std::endl;
    std::cout << "Navigation: [g]oto line" << std::endl;
    std::cout << "Other: [h]elp [c]lear [t]est [x]it" << std::endl;
    std::cout << "\nPress any key to continue...";
    waitForKey();
    showMenu_ = false;
}

void BetterUI::handleInput() {
    std::cout << "\nEnter command (h for help): ";
    std::string input;
    std::getline(std::cin, input);
    
    if (input.empty()) {
        return;
    }
    
    processCommand(input);
}

void BetterUI::processCommand(const std::string& cmd) {
    if (cmd.empty()) return;
    
    char cmdChar = cmd[0];
    
    switch (cmdChar) {
        case 'q':
        case 'x':
            running_ = false;
            break;
            
        case 'h':
            showMenu_ = true;
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
            insertText();
            break;
            
        case 'd':
            deleteText();
            break;
            
        case 'u':
            editor_->undo();
            showMessage("Undo performed");
            break;
            
        case 'r':
            editor_->redo();
            showMessage("Redo performed");
            break;
            
        case 'a':
            appendText();
            break;
            
        case 'f':
            searchText();
            break;
            
        case 'F':
            replaceText();
            break;
            
        case 'g':
            {
                std::string lineStr = getInput("Enter line number: ");
                if (!lineStr.empty()) {
                    try {
                        int lineNum = std::stoi(lineStr);
                        editor_->moveCursorToLine(lineNum - 1);
                        showMessage("Moved to line " + std::to_string(lineNum));
                    } catch (const std::exception& e) {
                        showError("Invalid line number");
                    }
                }
            }
            break;
            
        case 'c':
            clearScreen();
            break;
            
        case 't':
            testEditor();
            break;
            
        default:
            showError("Unknown command: " + cmd);
            break;
    }
}

void BetterUI::insertText() {
    std::string text = getInput("Enter text to insert: ");
    if (!text.empty()) {
        editor_->insertText(text);
        showMessage("Text inserted: " + text);
    }
}

void BetterUI::deleteText() {
    std::string lengthStr = getInput("Enter number of characters to delete: ");
    if (!lengthStr.empty()) {
        try {
            int length = std::stoi(lengthStr);
            editor_->deleteText(length);
            showMessage("Deleted " + std::to_string(length) + " characters");
        } catch (const std::exception& e) {
            showError("Invalid number");
        }
    }
}

void BetterUI::appendText() {
    std::string text = getInput("Enter text to append: ");
    if (!text.empty()) {
        editor_->insertText(text);
        showMessage("Text appended: " + text);
    }
}

void BetterUI::searchText() {
    std::string term = getInput("Search for: ");
    if (!term.empty()) {
        auto results = editor_->findText(term);
        showSearchResults(results);
    }
}

void BetterUI::replaceText() {
    std::string find = getInput("Find: ");
    if (!find.empty()) {
        std::string replace = getInput("Replace with: ");
        editor_->replaceText(find, replace, true);
        showMessage("Replace completed");
    }
}

void BetterUI::showSearchResults(const std::vector<int>& results) {
    if (results.empty()) {
        showMessage("No matches found");
    } else {
        std::cout << "\nFound " << results.size() << " matches at positions: ";
        for (size_t i = 0; i < results.size() && i < 10; i++) {
            std::cout << results[i] << " ";
        }
        if (results.size() > 10) {
            std::cout << "...";
        }
        std::cout << std::endl;
        showMessage("Search completed");
    }
}

void BetterUI::openFile() {
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

void BetterUI::saveFile() {
    if (editor_->saveDocument()) {
        showMessage("File saved: " + currentFile_);
    } else {
        showError("Failed to save file");
    }
}

void BetterUI::saveAsFile() {
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

void BetterUI::newFile() {
    editor_->newDocument();
    currentFile_ = "untitled.txt";
    showMessage("New document created");
}

void BetterUI::showMessage(const std::string& msg) {
    statusMessage_ = msg;
}

void BetterUI::showError(const std::string& error) {
    errorMessage_ = error;
}

std::string BetterUI::getInput(const std::string& prompt) {
    std::cout << prompt;
    std::string input;
    std::getline(std::cin, input);
    return input;
}

void BetterUI::waitForKey() {
    std::cin.get();
}

void BetterUI::testEditor() {
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
    
    showMessage("All tests completed successfully!");
}

} // namespace TextFlow
