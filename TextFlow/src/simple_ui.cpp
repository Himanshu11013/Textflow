#include "simple_ui.h"
t
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <unistd.h>
#include <sys/ioctl.h>

namespace TextFlow {

SimpleUI::SimpleUI() 
    : editor_(nullptr), running_(false), currentFile_("untitled.txt"),
      cursorX_(0), cursorY_(0), scrollX_(0), scrollY_(0),
      screenWidth_(80), screenHeight_(24) {
    
    // Get terminal size
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
        screenWidth_ = w.ws_col;
        screenHeight_ = w.ws_row;
    }
}

SimpleUI::~SimpleUI() {
    cleanup();
}

void SimpleUI::setTextEditor(std::shared_ptr<TextEditor> editor) {
    editor_ = editor;
}

void SimpleUI::run() {
    if (!editor_) {
        std::cerr << "Error: No text editor set!" << std::endl;
        return;
    }
    
    running_ = true;
    
    // Clear screen
    system("clear");
    
    std::cout << "TextFlow - Advanced Text Editor (Simple Mode)" << std::endl;
    std::cout << "=============================================" << std::endl;
    
    while (running_) {
        displayStatus();
        displayText();
        displayMenu();
        handleInput();
    }
    
    cleanup();
}

void SimpleUI::cleanup() {
    if (editor_) {
        editor_->saveDocument();
    }
}

void SimpleUI::displayStatus() {
    std::cout << "\n--- Status ---" << std::endl;
    std::cout << "File: " << currentFile_ << std::endl;
    std::cout << "Cursor: (" << cursorX_ << ", " << cursorY_ << ")" << std::endl;
    std::cout << "Lines: " << editor_->getLineCount() << std::endl;
    std::cout << "Current Line: " << editor_->getCurrentLine() << std::endl;
    std::cout << "Current Column: " << editor_->getCurrentColumn() << std::endl;
    std::cout << "Modified: " << (editor_->isModified() ? "Yes" : "No") << std::endl;
    std::cout << "NLP: OFF (Simplified)" << std::endl;
}

void SimpleUI::displayText() {
    std::cout << "\n--- Text Content ---" << std::endl;
    
    std::string text = editor_->getText();
    std::istringstream iss(text);
    std::string line;
    int lineNum = 1;
    
    while (std::getline(iss, line) && lineNum <= 20) {
        std::cout << std::setw(3) << lineNum << "| ";
        
        if (line.length() > 100) {
            line = line.substr(0, 97) + "...";
        }
        std::cout << line << std::endl;
        lineNum++;
    }
    
    if (lineNum > 20) {
        std::cout << "     ... (more lines available)" << std::endl;
    }
}

void SimpleUI::displayMenu() {
    std::cout << "\n--- Commands ---" << std::endl;
    std::cout << "File: [o]pen [s]ave [n]ew [q]uit" << std::endl;
    std::cout << "Edit: [i]nsert [d]elete [u]ndo [r]edo [a]ppend" << std::endl;
    std::cout << "Search: [f]ind [F]ind&Replace" << std::endl;
    std::cout << "Navigation: [g]oto line" << std::endl;
    std::cout << "Other: [h]elp [c]lear [t]est [x]it" << std::endl;
    std::cout << "\nEnter command: ";
}

void SimpleUI::handleInput() {
    std::string input;
    std::getline(std::cin, input);
    
    if (input.empty()) {
        return;
    }
    
    char cmd = input[0];
    
    switch (cmd) {
        case 'q':
        case 'x':
            running_ = false;
            break;
            
        case 'o': {
            showFileMenu();
            break;
        }
            
        case 's': {
            if (editor_->saveDocument()) {
                std::cout << "File saved successfully!" << std::endl;
            } else {
                std::cout << "Failed to save file!" << std::endl;
            }
            break;
        }
            
        case 'n': {
            editor_->newDocument();
            currentFile_ = "untitled.txt";
            cursorX_ = cursorY_ = 0;
            std::cout << "New document created." << std::endl;
            break;
        }
            
        case 'i': {
            std::cout << "Enter text to insert: ";
            std::string text;
            std::getline(std::cin, text);
            editor_->insertText(text);
            std::cout << "Text inserted." << std::endl;
            break;
        }
            
        case 'd': {
            editor_->deleteText(1);
            std::cout << "Character deleted." << std::endl;
            break;
        }
            
        case 'u': {
            editor_->undo();
            std::cout << "Undo performed." << std::endl;
            break;
        }
            
        case 'r': {
            editor_->redo();
            std::cout << "Redo performed." << std::endl;
            break;
        }
            
        case 'a': {
            std::cout << "Enter text to append: ";
            std::string appendText;
            std::getline(std::cin, appendText);
            editor_->insertText(appendText);
            std::cout << "Text appended." << std::endl;
            break;
        }
            
        case 'f': {
            std::cout << "Enter search term: ";
            std::string searchTerm;
            std::getline(std::cin, searchTerm);
            auto results = editor_->findText(searchTerm);
            std::cout << "Found " << results.size() << " matches at positions: ";
            for (size_t i = 0; i < results.size() && i < 10; i++) {
                std::cout << results[i] << " ";
            }
            if (results.size() > 10) {
                std::cout << "...";
            }
            std::cout << std::endl;
            break;
        }
            
        case 'F': {
            std::cout << "Enter search term: ";
            std::string findTerm;
            std::getline(std::cin, findTerm);
            std::cout << "Enter replacement: ";
            std::string replaceTerm;
            std::getline(std::cin, replaceTerm);
            editor_->replaceText(findTerm, replaceTerm, true);
            std::cout << "Replace operation completed." << std::endl;
            break;
        }
            
        case 'h': {
            showHelp();
            break;
        }
            
        case 'g': {
            std::cout << "Enter line number: ";
            int lineNum;
            if (std::cin >> lineNum) {
                editor_->moveCursorToLine(lineNum - 1);
                std::cout << "Moved to line " << lineNum << std::endl;
            }
            std::cin.ignore(); // Clear input buffer
            break;
        }
            
        case 't': {
            testEditor();
            break;
        }
            
        case 'c': {
            system("clear");
            break;
        }
            
        default:
            std::cout << "Unknown command: " << cmd << std::endl;
            break;
    }
}

void SimpleUI::showHelp() {
    std::cout << "\n--- TextFlow Help ---" << std::endl;
    std::cout << "This is a simplified console interface for TextFlow." << std::endl;
    std::cout << "The editor uses an AVL tree for efficient text storage." << std::endl;
    std::cout << "\nKey Features:" << std::endl;
    std::cout << "- O(log n) insert/delete operations" << std::endl;
    std::cout << "- Undo/Redo functionality" << std::endl;
    std::cout << "- Search and replace" << std::endl;
    std::cout << "- File I/O operations" << std::endl;
    std::cout << "- Text compression (Huffman coding)" << std::endl;
    std::cout << "\nCommands:" << std::endl;
    std::cout << "File operations: o(pen), s(ave), n(ew), q(uit)" << std::endl;
    std::cout << "Edit operations: i(nsert), d(elete), u(ndo), r(edo), a(ppend)" << std::endl;
    std::cout << "Search operations: f(ind), F(ind&Replace)" << std::endl;
    std::cout << "Navigation: g(oto line), h(elp), c(lear), t(est), x(it)" << std::endl;
}

void SimpleUI::showFileMenu() {
    std::cout << "\n--- File Menu ---" << std::endl;
    std::cout << "1. Open file" << std::endl;
    std::cout << "2. Save as" << std::endl;
    std::cout << "3. Back to main menu" << std::endl;
    std::cout << "Choice: ";
    
    int choice;
    if (std::cin >> choice) {
        std::cin.ignore(); // Clear input buffer
        
        switch (choice) {
            case 1: {
                std::cout << "Enter filename: ";
                std::string filename;
                std::getline(std::cin, filename);
                if (editor_->openDocument(filename)) {
                    currentFile_ = filename;
                    cursorX_ = cursorY_ = 0;
                    std::cout << "File opened: " << filename << std::endl;
                } else {
                    std::cout << "Failed to open file: " << filename << std::endl;
                }
                break;
            }
            case 2: {
                std::cout << "Enter filename: ";
                std::string filename;
                std::getline(std::cin, filename);
                if (editor_->saveAsDocument(filename)) {
                    currentFile_ = filename;
                    std::cout << "File saved as: " << filename << std::endl;
                } else {
                    std::cout << "Failed to save file: " << filename << std::endl;
                }
                break;
            }
            case 3:
                break;
            default:
                std::cout << "Invalid choice!" << std::endl;
                break;
        }
    }
}

void SimpleUI::testEditor() {
    std::cout << "\n--- Testing Editor Features ---" << std::endl;
    
    // Test basic operations
    std::cout << "Testing basic text operations..." << std::endl;
    editor_->insertText("Hello, World!\nThis is a test line.\nLine 1: More text here.");
    
    std::cout << "Line count: " << editor_->getLineCount() << std::endl;
    std::cout << "Current line: " << editor_->getCurrentLine() << std::endl;
    std::cout << "Current column: " << editor_->getCurrentColumn() << std::endl;
    
    // Test search
    auto results = editor_->findText("test");
    std::cout << "Search for 'test': " << results.size() << " matches" << std::endl;
    
    // Test undo/redo
    std::cout << "Testing undo..." << std::endl;
    editor_->undo();
    std::cout << "After undo - Line count: " << editor_->getLineCount() << std::endl;
    
    editor_->redo();
    std::cout << "After redo - Line count: " << editor_->getLineCount() << std::endl;
    
    std::cout << "Test completed!" << std::endl;
}

} // namespace TextFlow
