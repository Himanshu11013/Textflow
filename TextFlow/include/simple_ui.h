#pragma once

#include "text_editor.h"
#include <memory>
#include <string>
#include <vector>

namespace TextFlow {

class SimpleUI {
public:
    SimpleUI();
    ~SimpleUI();
    
    void setTextEditor(std::shared_ptr<TextEditor> editor);
    void run();
    void cleanup();
    
private:
    void displayMenu();
    void displayText();
    void displayStatus();
    void handleInput();
    void showHelp();
    void showFileMenu();
    void showEditMenu();
    void showSearchMenu();
    void testEditor();
    
    std::shared_ptr<TextEditor> editor_;
    bool running_;
    std::string currentFile_;
    int cursorX_, cursorY_;
    int scrollX_, scrollY_;
    int screenWidth_, screenHeight_;
};

} // namespace TextFlow
