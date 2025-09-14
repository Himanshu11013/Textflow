#pragma once

#include "text_editor.h"
#include <memory>
#include <string>
#include <vector>
#include <termios.h>

namespace TextFlow {

class ImprovedUI {
public:
    ImprovedUI();
    ~ImprovedUI();
    
    void setTextEditor(std::shared_ptr<TextEditor> editor);
    void run();
    void cleanup();
    
private:
    // Display functions
    void clearScreen();
    void displayHeader();
    void displayText();
    void displayStatusBar();
    void displayMenu();
    
    // Input handling
    void handleInput();
    void handleKeyPress();
    void handleCommand();
    
    // Text editing
    void insertChar(char c);
    void deleteChar();
    void moveCursor(int dx, int dy);
    void moveToLine(int line);
    void moveToColumn(int col);
    
    // Search and replace
    void searchText();
    void replaceText();
    void highlightSearchResults();
    
    // File operations
    void openFile();
    void saveFile();
    void saveAsFile();
    void newFile();
    
    // Utility functions
    void showMessage(const std::string& msg);
    void showError(const std::string& error);
    std::string getInput(const std::string& prompt);
    void setupTerminal();
    void restoreTerminal();
    void testEditor();
    
    // Member variables
    std::shared_ptr<TextEditor> editor_;
    bool running_;
    std::string currentFile_;
    std::string statusMessage_;
    std::string errorMessage_;
    
    // Cursor and display
    int cursorX_, cursorY_;
    int scrollX_, scrollY_;
    int screenWidth_, screenHeight_;
    int textStartY_;
    
    // Search
    std::string searchTerm_;
    std::vector<int> searchResults_;
    int currentSearchIndex_;
    bool searchActive_;
    
    // Terminal settings
    struct termios originalTermios_;
    bool terminalConfigured_;
    
    // UI state
    bool showMenu_;
    bool inCommandMode_;
    std::string commandBuffer_;
};

} // namespace TextFlow
