#pragma once

#include "text_editor.h"
#include <memory>
#include <string>
#include <vector>

namespace TextFlow {

class BetterUI {
public:
    BetterUI();
    ~BetterUI();
    
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
    void processCommand(const std::string& cmd);
    
    // Text editing
    void insertText();
    void deleteText();
    void appendText();
    
    // Search and replace
    void searchText();
    void replaceText();
    void showSearchResults(const std::vector<int>& results);
    
    // File operations
    void openFile();
    void saveFile();
    void saveAsFile();
    void newFile();
    
    // Utility functions
    void showMessage(const std::string& msg);
    void showError(const std::string& error);
    std::string getInput(const std::string& prompt);
    void waitForKey();
    void testEditor();
    
    // Member variables
    std::shared_ptr<TextEditor> editor_;
    bool running_;
    std::string currentFile_;
    std::string statusMessage_;
    std::string errorMessage_;
    
    // Display
    int screenWidth_, screenHeight_;
    bool showMenu_;
};

} // namespace TextFlow
