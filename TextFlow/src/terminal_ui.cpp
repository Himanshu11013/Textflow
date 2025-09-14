#include "terminal_ui.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <signal.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace TextFlow {

// Global UI instance
std::unique_ptr<TerminalUI> g_ui = nullptr;

TerminalUI::TerminalUI() 
    : running_(false), initialized_(false), screenHeight_(0), screenWidth_(0),
      textAreaHeight_(0), textAreaWidth_(0), textStartX_(0), textStartY_(0),
      mainWindow_(nullptr), textWindow_(nullptr), statusWindow_(nullptr),
      menuWindow_(nullptr), lineNumberWindow_(nullptr),
      scrollX_(0), scrollY_(0), cursorX_(0), cursorY_(0),
      showLineNumbers_(true), wordWrap_(true), syntaxHighlighting_(true),
      autoComplete_(true), nlpFeatures_(true),
      currentTheme_(Theme::DARK), inInputMode_(false),
      suggestionIndex_(0), showSuggestions_(false) {
    initializeThemes();
}

TerminalUI::~TerminalUI() {
    cleanup();
}

bool TerminalUI::initialize() {
    if (initialized_) {
        return true;
    }
    
    // Initialize ncurses
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0); // Hide cursor initially
    
    // Enable mouse support
    mousemask(ALL_MOUSE_EVENTS, nullptr);
    
    // Get screen dimensions
    getmaxyx(stdscr, screenHeight_, screenWidth_);
    
    // Initialize colors
    if (has_colors()) {
        start_color();
        setupColors();
    }
    
    // Create windows
    createWindows();
    
    // Set up signal handlers
    setupSignalHandlers();
    
    initialized_ = true;
    running_ = true;
    
    return true;
}

void TerminalUI::cleanup() {
    if (!initialized_) {
        return;
    }
    
    running_ = false;
    
    // Destroy windows
    destroyWindows();
    
    // Cleanup ncurses
    endwin();
    
    initialized_ = false;
}

void TerminalUI::run() {
    if (!initialize()) {
        std::cerr << "Failed to initialize terminal UI" << std::endl;
        return;
    }
    
    mainLoop();
    cleanup();
}

void TerminalUI::setTheme(Theme theme) {
    currentTheme_ = theme;
    colors_ = themes_[theme];
    setupColors();
    refreshAll();
}

void TerminalUI::loadTheme(const std::string& themeName) {
    // Load theme from file (simplified implementation)
    // In a full implementation, this would load from a configuration file
    if (themeName == "dark") {
        setTheme(Theme::DARK);
    } else if (themeName == "light") {
        setTheme(Theme::LIGHT);
    } else if (themeName == "monokai") {
        setTheme(Theme::MONOKAI);
    } else if (themeName == "solarized_dark") {
        setTheme(Theme::SOLARIZED_DARK);
    } else if (themeName == "solarized_light") {
        setTheme(Theme::SOLARIZED_LIGHT);
    }
}

void TerminalUI::saveTheme(const std::string& themeName) {
    // Save theme to file (simplified implementation)
    // In a full implementation, this would save to a configuration file
    logMessage("Theme " + themeName + " saved");
}

void TerminalUI::createWindows() {
    calculateDimensions();
    
    // Create main window
    mainWindow_ = newwin(screenHeight_, screenWidth_, 0, 0);
    wbkgd(mainWindow_, COLOR_PAIR(1));
    
    // Create menu bar
    menuWindow_ = newwin(1, screenWidth_, 0, 0);
    wbkgd(menuWindow_, COLOR_PAIR(2));
    
    // Create text area
    textWindow_ = newwin(textAreaHeight_, textAreaWidth_, textStartY_, textStartX_);
    wbkgd(textWindow_, COLOR_PAIR(1));
    scrollok(textWindow_, TRUE);
    
    // Create line number window
    if (showLineNumbers_) {
        lineNumberWindow_ = newwin(textAreaHeight_, 8, textStartY_, 0);
        wbkgd(lineNumberWindow_, COLOR_PAIR(3));
    }
    
    // Create status bar
    statusWindow_ = newwin(1, screenWidth_, screenHeight_ - 1, 0);
    wbkgd(statusWindow_, COLOR_PAIR(2));
}

void TerminalUI::destroyWindows() {
    if (mainWindow_) {
        delwin(mainWindow_);
        mainWindow_ = nullptr;
    }
    if (textWindow_) {
        delwin(textWindow_);
        textWindow_ = nullptr;
    }
    if (statusWindow_) {
        delwin(statusWindow_);
        statusWindow_ = nullptr;
    }
    if (menuWindow_) {
        delwin(menuWindow_);
        menuWindow_ = nullptr;
    }
    if (lineNumberWindow_) {
        delwin(lineNumberWindow_);
        lineNumberWindow_ = nullptr;
    }
}

void TerminalUI::refreshAll() {
    if (!initialized_) return;
    
    wrefresh(mainWindow_);
    wrefresh(textWindow_);
    wrefresh(statusWindow_);
    wrefresh(menuWindow_);
    if (lineNumberWindow_) {
        wrefresh(lineNumberWindow_);
    }
}

void TerminalUI::resizeHandler() {
    getmaxyx(stdscr, screenHeight_, screenWidth_);
    destroyWindows();
    createWindows();
    refreshAll();
}

void TerminalUI::mainLoop() {
    while (running_) {
        updateDisplay();
        
        int ch = getch();
        if (ch == KEY_RESIZE) {
            resizeHandler();
            continue;
        }
        
        handleKeyPress(ch);
    }
}

void TerminalUI::handleInput() {
    // This is called from handleKeyPress for character input
    if (editor_) {
        std::string input(1, static_cast<char>(getch()));
        editor_->insertText(input);
        onTextChanged();
    }
}

void TerminalUI::updateDisplay() {
    if (!initialized_) return;
    
    // Clear windows
    werase(textWindow_);
    if (lineNumberWindow_) {
        werase(lineNumberWindow_);
    }
    werase(statusWindow_);
    werase(menuWindow_);
    
    // Display content
    displayText();
    displayLineNumbers();
    displayStatusBar();
    displayMenuBar();
    
    // Display suggestions if enabled
    if (showSuggestions_ && !suggestions_.empty()) {
        displaySuggestions();
    }
    
    refreshAll();
}

void TerminalUI::setTextEditor(std::shared_ptr<TextEditor> editor) {
    editor_ = editor;
}

// NLP integration removed for simplified version

void TerminalUI::displayText() {
    if (!editor_ || !textWindow_) return;
    
    int currentLine = editor_->getCurrentLine();
    int totalLines = editor_->getLineCount();
    
    // Calculate visible range
    int startLine = std::max(1, currentLine - textAreaHeight_ / 2);
    int endLine = std::min(totalLines, startLine + textAreaHeight_ - 1);
    
    int y = 0;
    for (int line = startLine; line <= endLine; line++) {
        std::string lineText = editor_->getLine(line);
        
        // Apply word wrap if enabled
        if (wordWrap_ && lineText.length() > textAreaWidth_) {
            // Simple word wrapping
            std::vector<std::string> wrapped;
            std::string currentLine = "";
            std::istringstream iss(lineText);
            std::string word;
            
            while (iss >> word) {
                if (currentLine.length() + word.length() + 1 <= static_cast<size_t>(textAreaWidth_)) {
                    if (!currentLine.empty()) currentLine += " ";
                    currentLine += word;
                } else {
                    if (!currentLine.empty()) {
                        wrapped.push_back(currentLine);
                        currentLine = word;
                    } else {
                        wrapped.push_back(word);
                    }
                }
            }
            if (!currentLine.empty()) {
                wrapped.push_back(currentLine);
            }
            for (const auto& wrappedLine : wrapped) {
                if (y >= textAreaHeight_) break;
                
                mvwaddstr(textWindow_, y, 0, wrappedLine.c_str());
                y++;
            }
        } else {
            if (y >= textAreaHeight_) break;
            
            // Truncate if too long
            if (lineText.length() > textAreaWidth_) {
                lineText = lineText.substr(0, textAreaWidth_ - 3) + "...";
            }
            
            mvwaddstr(textWindow_, y, 0, lineText.c_str());
            y++;
        }
    }
    
    // Display cursor
    displayCursor();
    
    // Display selection
    if (editor_->hasSelection()) {
        displaySelection();
    }
}

void TerminalUI::displayStatusBar() {
    if (!statusWindow_ || !editor_) return;
    
    std::string status = editor_->getStatusText();
    
    // Add additional status information
    status += " | NLP: OFF (Simplified)";
    
    // Add theme information
    status += " | Theme: " + std::to_string(static_cast<int>(currentTheme_));
    
    // Truncate if too long
    if (status.length() > screenWidth_) {
        status = status.substr(0, screenWidth_ - 3) + "...";
    }
    
    mvwaddstr(statusWindow_, 0, 0, status.c_str());
}

void TerminalUI::displayMenuBar() {
    if (!menuWindow_) return;
    
    std::string menu = "File | Edit | View | Search | Tools | Help";
    mvwaddstr(menuWindow_, 0, 0, menu.c_str());
}

void TerminalUI::displayLineNumbers() {
    if (!lineNumberWindow_ || !showLineNumbers_) return;
    
    if (!editor_) return;
    
    int currentLine = editor_->getCurrentLine();
    int totalLines = editor_->getLineCount();
    
    int startLine = std::max(1, currentLine - textAreaHeight_ / 2);
    int endLine = std::min(totalLines, startLine + textAreaHeight_ - 1);
    
    int y = 0;
    for (int line = startLine; line <= endLine; line++) {
        std::string lineNum = std::to_string(line);
        // Right-align line numbers
        int padding = 7 - lineNum.length();
        std::string padded = std::string(padding, ' ') + lineNum + " ";
        
        mvwaddstr(lineNumberWindow_, y, 0, padded.c_str());
        y++;
    }
}

void TerminalUI::displayCursor() {
    if (!textWindow_ || !editor_) return;
    
    int cursorX = editor_->getCurrentColumn() - 1;
    int cursorY = editor_->getCurrentLine() - 1;
    
    // Calculate display position
    int displayY = cursorY - (editor_->getCurrentLine() - textAreaHeight_ / 2);
    int displayX = cursorX;
    
    if (displayY >= 0 && displayY < textAreaHeight_ && displayX >= 0 && displayX < textAreaWidth_) {
        wmove(textWindow_, displayY, displayX);
        wchgat(textWindow_, 1, A_REVERSE, 0, nullptr);
    }
}

void TerminalUI::displaySelection() {
    if (!textWindow_ || !editor_ || !editor_->hasSelection()) return;
    
    // Implementation for highlighting selected text
    // This is a simplified version
    int startX = editor_->getSelectionStart();
    int endX = editor_->getSelectionEnd();
    
    // Calculate display positions and highlight
    // (Simplified implementation)
}

void TerminalUI::handleKeyPress(int key) {
    if (!editor_) return;
    
    switch (key) {
        case KEY_UP:
            editor_->moveCursorUp();
            break;
        case KEY_DOWN:
            editor_->moveCursorDown();
            break;
        case KEY_LEFT:
            editor_->moveCursorLeft();
            break;
        case KEY_RIGHT:
            editor_->moveCursorRight();
            break;
        case KEY_HOME:
            editor_->moveCursorToLineStart();
            break;
        case KEY_END:
            editor_->moveCursorToLineEnd();
            break;
        case KEY_PPAGE:
            for (int i = 0; i < textAreaHeight_; i++) {
                editor_->moveCursorUp();
            }
            break;
        case KEY_NPAGE:
            for (int i = 0; i < textAreaHeight_; i++) {
                editor_->moveCursorDown();
            }
            break;
        case 127: // Backspace
        case KEY_BACKSPACE:
            editor_->backspace(1);
            onTextChanged();
            break;
        case KEY_DC: // Delete
            editor_->deleteText(1);
            onTextChanged();
            break;
        case '\n':
        case '\r':
            editor_->insertNewline();
            onTextChanged();
            break;
        case '\t':
            editor_->insertTab();
            onTextChanged();
            break;
        case 27: // Escape
            if (showSuggestions_) {
                showSuggestions_ = false;
                suggestions_.clear();
            }
            break;
        case 'q':
            if (getConfirmation("Are you sure you want to quit?")) {
                running_ = false;
            }
            break;
        case 'o':
            showOpenDialog();
            break;
        case 's':
            if (getch() == 's') { // Ctrl+S
                saveFile();
            } else {
                saveAsFile();
            }
            break;
        case 'n':
            newFile();
            break;
        case 'f':
            findText();
            break;
        case 'r':
            replaceText();
            break;
        case 'g':
            gotoLine();
            break;
        case 'z':
            editor_->undo();
            onTextChanged();
            break;
        case 'y':
            editor_->redo();
            onTextChanged();
            break;
        case 'a':
            editor_->selectAll();
            break;
        case 'c':
            editor_->copy();
            break;
        case 'x':
            editor_->cut();
            onTextChanged();
            break;
        case 'v':
            editor_->paste();
            onTextChanged();
            break;
        default:
            if (key >= 32 && key <= 126) { // Printable characters
                editor_->insertText(std::string(1, static_cast<char>(key)));
                onTextChanged();
            }
            break;
    }
    
    onCursorMoved();
}

void TerminalUI::handleSpecialKeys(int key) {
    // Handle special key combinations
    // This would be expanded for more complex key handling
}

void TerminalUI::handleMouse(int x, int y) {
    // Handle mouse events
    // This would be implemented for mouse support
}

void TerminalUI::showOpenDialog() {
    std::string filename = getInput("Open file: ");
    if (!filename.empty()) {
        if (editor_->openDocument(filename)) {
            onFileOpened(filename);
            showSuccess("File opened: " + filename);
        } else {
            showError("Failed to open file: " + filename);
        }
    }
}

void TerminalUI::showSaveDialog() {
    if (editor_->saveDocument()) {
        showSuccess("File saved");
    } else {
        showError("Failed to save file");
    }
}

void TerminalUI::showFindDialog() {
    std::string searchText = getInput("Find: ", lastSearch_);
    if (!searchText.empty()) {
        lastSearch_ = searchText;
        auto results = editor_->findText(searchText);
        if (!results.empty()) {
            showStatus("Found " + std::to_string(results.size()) + " matches");
        } else {
            showStatus("No matches found");
        }
    }
}

void TerminalUI::showReplaceDialog() {
    std::string searchText = getInput("Find: ", lastSearch_);
    if (searchText.empty()) return;
    
    std::string replaceText = getInput("Replace with: ", lastReplace_);
    
    lastSearch_ = searchText;
    lastReplace_ = replaceText;
    
    editor_->replaceText(searchText, replaceText, true);
    onTextChanged();
    showSuccess("Replaced all occurrences");
}

void TerminalUI::showGotoDialog() {
    std::string lineStr = getInput("Go to line: ");
    if (!lineStr.empty()) {
        try {
            int line = std::stoi(lineStr);
            editor_->moveCursorToLine(line);
            onCursorMoved();
        } catch (const std::exception& e) {
            showError("Invalid line number");
        }
    }
}

void TerminalUI::showThemeDialog() {
    std::vector<std::string> themes = {"Dark", "Light", "Monokai", "Solarized Dark", "Solarized Light"};
    int choice = getChoice("Select theme:", themes);
    
    if (choice >= 0) {
        setTheme(static_cast<Theme>(choice));
        showSuccess("Theme changed");
    }
}

void TerminalUI::showNLPDialog() {
    showError("NLP service not available in simplified version");
    return;
    
    std::vector<std::string> options = {"Grammar Check", "Summarize", "Extract Keywords", "Sentiment Analysis", "Readability Check"};
    int choice = getChoice("NLP Features:", options);
    
    if (choice >= 0 && editor_) {
        std::string text = editor_->getText();
        if (text.empty()) {
            showWarning("No text to process");
            return;
        }
        
        processTextWithNLP();
    }
}

void TerminalUI::showStatus(const std::string& message, int timeout) {
    // Display status message
    // This would be implemented with a status message system
    logMessage("Status: " + message);
}

void TerminalUI::showError(const std::string& error) {
    // Display error message
    logMessage("Error: " + error);
}

void TerminalUI::showWarning(const std::string& warning) {
    // Display warning message
    logMessage("Warning: " + warning);
}

void TerminalUI::showSuccess(const std::string& message) {
    // Display success message
    logMessage("Success: " + message);
}

void TerminalUI::setLineNumbers(bool enabled) {
    showLineNumbers_ = enabled;
    if (enabled && !lineNumberWindow_) {
        lineNumberWindow_ = newwin(textAreaHeight_, 8, textStartY_, 0);
        wbkgd(lineNumberWindow_, COLOR_PAIR(3));
    } else if (!enabled && lineNumberWindow_) {
        delwin(lineNumberWindow_);
        lineNumberWindow_ = nullptr;
    }
}

void TerminalUI::setWordWrap(bool enabled) {
    wordWrap_ = enabled;
}

void TerminalUI::setSyntaxHighlighting(bool enabled) {
    syntaxHighlighting_ = enabled;
}

void TerminalUI::setAutoComplete(bool enabled) {
    autoComplete_ = enabled;
}

void TerminalUI::setNLPFeatures(bool enabled) {
    nlpFeatures_ = enabled;
}

void TerminalUI::initializeThemes() {
    // Dark theme
    themes_[Theme::DARK] = {
        COLOR_BLACK,    // background
        COLOR_WHITE,    // foreground
        COLOR_CYAN,     // cursor
        COLOR_BLUE,     // selection
        COLOR_BLACK,    // status_bar
        COLOR_BLACK,    // menu_bar
        COLOR_RED,      // error
        COLOR_YELLOW,   // warning
        COLOR_GREEN,    // success
        COLOR_MAGENTA   // highlight
    };
    
    // Light theme
    themes_[Theme::LIGHT] = {
        COLOR_WHITE,    // background
        COLOR_BLACK,    // foreground
        COLOR_BLUE,     // cursor
        COLOR_CYAN,     // selection
        COLOR_WHITE,    // status_bar
        COLOR_WHITE,    // menu_bar
        COLOR_RED,      // error
        COLOR_YELLOW,   // warning
        COLOR_GREEN,    // success
        COLOR_MAGENTA   // highlight
    };
    
    // Monokai theme
    themes_[Theme::MONOKAI] = {
        COLOR_BLACK,    // background
        COLOR_WHITE,    // foreground
        COLOR_YELLOW,   // cursor
        COLOR_MAGENTA,  // selection
        COLOR_BLACK,    // status_bar
        COLOR_BLACK,    // menu_bar
        COLOR_RED,      // error
        COLOR_YELLOW,   // warning
        COLOR_GREEN,    // success
        COLOR_CYAN      // highlight
    };
    
    // Solarized Dark theme
    themes_[Theme::SOLARIZED_DARK] = {
        COLOR_BLACK,    // background
        COLOR_WHITE,    // foreground
        COLOR_CYAN,     // cursor
        COLOR_BLUE,     // selection
        COLOR_BLACK,    // status_bar
        COLOR_BLACK,    // menu_bar
        COLOR_RED,      // error
        COLOR_YELLOW,   // warning
        COLOR_GREEN,    // success
        COLOR_MAGENTA   // highlight
    };
    
    // Solarized Light theme
    themes_[Theme::SOLARIZED_LIGHT] = {
        COLOR_WHITE,    // background
        COLOR_BLACK,    // foreground
        COLOR_BLUE,     // cursor
        COLOR_CYAN,     // selection
        COLOR_WHITE,    // status_bar
        COLOR_WHITE,    // menu_bar
        COLOR_RED,      // error
        COLOR_YELLOW,   // warning
        COLOR_GREEN,    // success
        COLOR_MAGENTA   // highlight
    };
    
    colors_ = themes_[currentTheme_];
}

void TerminalUI::setupColors() {
    if (!has_colors()) return;
    
    // Initialize color pairs
    init_pair(1, colors_.foreground, colors_.background);
    init_pair(2, colors_.foreground, colors_.status_bar);
    init_pair(3, colors_.foreground, colors_.menu_bar);
    init_pair(4, colors_.cursor, colors_.background);
    init_pair(5, colors_.selection, colors_.background);
    init_pair(6, colors_.error, colors_.background);
    init_pair(7, colors_.warning, colors_.background);
    init_pair(8, colors_.success, colors_.background);
    init_pair(9, colors_.highlight, colors_.background);
}

void TerminalUI::calculateDimensions() {
    screenHeight_ = LINES;
    screenWidth_ = COLS;
    
    textAreaHeight_ = screenHeight_ - 2; // Account for status and menu bars
    textAreaWidth_ = showLineNumbers_ ? screenWidth_ - 8 : screenWidth_;
    textStartX_ = showLineNumbers_ ? 8 : 0;
    textStartY_ = 1; // Account for menu bar
}

void TerminalUI::updateCursorPosition() {
    if (!editor_) return;
    
    cursorX_ = editor_->getCurrentColumn() - 1;
    cursorY_ = editor_->getCurrentLine() - 1;
}

void TerminalUI::scrollToCursor() {
    if (!editor_) return;
    
    int currentLine = editor_->getCurrentLine();
    int totalLines = editor_->getLineCount();
    
    // Calculate scroll position to keep cursor visible
    int centerLine = textAreaHeight_ / 2;
    int targetScrollY = std::max(1, currentLine - centerLine);
    targetScrollY = std::min(targetScrollY, totalLines - textAreaHeight_ + 1);
    
    scrollY_ = targetScrollY;
}

void TerminalUI::scrollToPosition(int x, int y) {
    scrollX_ = x;
    scrollY_ = y;
}

void TerminalUI::renderTextLine(int line, int startCol, int endCol) {
    if (!editor_ || !textWindow_) return;
    
    std::string lineText = editor_->getLine(line);
    
    // Apply syntax highlighting if enabled
    if (syntaxHighlighting_) {
        highlightSyntax(lineText, line, 0);
    } else {
        mvwaddstr(textWindow_, line, 0, lineText.c_str());
    }
}

void TerminalUI::renderLineNumbers() {
    if (!lineNumberWindow_ || !showLineNumbers_) return;
    
    // Implementation for rendering line numbers
    // This is handled in displayLineNumbers()
}

void TerminalUI::highlightSyntax(const std::string& text, int y, int x) {
    // Simple syntax highlighting implementation
    // In a full implementation, this would parse the text and apply appropriate colors
    
    // Keywords
    std::vector<std::string> keywords = {
        "if", "else", "for", "while", "do", "switch", "case", "break", "continue",
        "return", "class", "struct", "enum", "namespace", "using", "include",
        "public", "private", "protected", "static", "const", "virtual", "override"
    };
    
    std::string lowerText = text;
    std::transform(lowerText.begin(), lowerText.end(), lowerText.begin(), ::tolower);
    
    for (const auto& keyword : keywords) {
        size_t pos = 0;
        while ((pos = lowerText.find(keyword, pos)) != std::string::npos) {
            // Check if it's a whole word
            if ((pos == 0 || !std::isalnum(lowerText[pos - 1])) &&
                (pos + keyword.length() == lowerText.length() || !std::isalnum(lowerText[pos + keyword.length()]))) {
                
                // Apply keyword highlighting
                wmove(textWindow_, y, x + pos);
                wchgat(textWindow_, keyword.length(), A_BOLD, 9, nullptr);
            }
            pos += keyword.length();
        }
    }
}

void TerminalUI::highlightSelection(int startX, int startY, int endX, int endY) {
    // Implementation for highlighting selected text
    // This would highlight the selected region
}

std::string TerminalUI::getInput(const std::string& prompt, const std::string& defaultValue) {
    // Create input dialog
    int width = std::max(40, static_cast<int>(prompt.length() + 20));
    int height = 5;
    int x = (screenWidth_ - width) / 2;
    int y = (screenHeight_ - height) / 2;
    
    WINDOW* inputWin = newwin(height, width, y, x);
    wbkgd(inputWin, COLOR_PAIR(1));
    box(inputWin, 0, 0);
    
    mvwaddstr(inputWin, 1, 1, prompt.c_str());
    
    // Input field
    std::string input = defaultValue;
    int inputX = 1;
    int inputY = 2;
    
    wmove(inputWin, inputY, inputX);
    waddstr(inputWin, input.c_str());
    
    wrefresh(inputWin);
    
    // Get input
    echo();
    curs_set(1);
    
    char buffer[256];
    wgetnstr(inputWin, buffer, sizeof(buffer) - 1);
    input = buffer;
    
    noecho();
    curs_set(0);
    
    delwin(inputWin);
    
    return input;
}

int TerminalUI::getChoice(const std::string& prompt, const std::vector<std::string>& options) {
    // Create choice dialog
    int width = std::max(40, static_cast<int>(prompt.length() + 10));
    int height = options.size() + 4;
    int x = (screenWidth_ - width) / 2;
    int y = (screenHeight_ - height) / 2;
    
    WINDOW* choiceWin = newwin(height, width, y, x);
    wbkgd(choiceWin, COLOR_PAIR(1));
    box(choiceWin, 0, 0);
    
    mvwaddstr(choiceWin, 1, 1, prompt.c_str());
    
    int selected = 0;
    int key;
    
    do {
        // Clear options area
        for (int i = 0; i < static_cast<int>(options.size()); i++) {
            mvwaddstr(choiceWin, i + 2, 1, std::string(width - 2, ' ').c_str());
        }
        
        // Display options
        for (int i = 0; i < static_cast<int>(options.size()); i++) {
            std::string option = (i == selected ? "> " : "  ") + options[i];
            mvwaddstr(choiceWin, i + 2, 1, option.c_str());
        }
        
        wrefresh(choiceWin);
        
        key = getch();
        
        switch (key) {
            case KEY_UP:
                selected = (selected - 1 + options.size()) % options.size();
                break;
            case KEY_DOWN:
                selected = (selected + 1) % options.size();
                break;
            case '\n':
            case '\r':
                delwin(choiceWin);
                return selected;
            case 27: // Escape
                delwin(choiceWin);
                return -1;
        }
    } while (true);
}

bool TerminalUI::getConfirmation(const std::string& message) {
    std::vector<std::string> options = {"Yes", "No"};
    int choice = getChoice(message, options);
    return choice == 0;
}

void TerminalUI::drawDialog(int x, int y, int width, int height, const std::string& title) {
    WINDOW* dialog = newwin(height, width, y, x);
    wbkgd(dialog, COLOR_PAIR(1));
    box(dialog, 0, 0);
    
    mvwaddstr(dialog, 0, 2, title.c_str());
    
    wrefresh(dialog);
}

void TerminalUI::drawInputField(int x, int y, int width, const std::string& text, bool selected) {
    // Implementation for drawing input fields
}

void TerminalUI::drawButton(int x, int y, int width, const std::string& text, bool selected) {
    // Implementation for drawing buttons
}

void TerminalUI::updateSuggestions() {
    // NLP suggestions disabled in simplified version
    suggestions_.clear();
    showSuggestions_ = false;
}

void TerminalUI::displaySuggestions() {
    if (suggestions_.empty() || !showSuggestions_) return;
    
    // Display suggestions in a popup
    int width = 30;
    int height = std::min(static_cast<int>(suggestions_.size()), 5) + 2;
    int x = screenWidth_ - width - 2;
    int y = 1;
    
    WINDOW* suggestionWin = newwin(height, width, y, x);
    wbkgd(suggestionWin, COLOR_PAIR(1));
    box(suggestionWin, 0, 0);
    
    for (int i = 0; i < std::min(static_cast<int>(suggestions_.size()), 5); i++) {
        std::string suggestion = (i == suggestionIndex_ ? "> " : "  ") + suggestions_[i];
        mvwaddstr(suggestionWin, i + 1, 1, suggestion.c_str());
    }
    
    wrefresh(suggestionWin);
}

void TerminalUI::applySuggestion(int index) {
    if (index < 0 || index >= static_cast<int>(suggestions_.size())) return;
    
    if (editor_) {
        editor_->insertText(suggestions_[index]);
        onTextChanged();
    }
    
    showSuggestions_ = false;
    suggestions_.clear();
}

void TerminalUI::processTextWithNLP() {
    // NLP processing disabled in simplified version
    showStatus("NLP processing not available in simplified version");
}

void TerminalUI::openFile() {
    showOpenDialog();
}

void TerminalUI::saveFile() {
    showSaveDialog();
}

void TerminalUI::saveAsFile() {
    std::string filename = getInput("Save as: ");
    if (!filename.empty()) {
        if (editor_->saveAsDocument(filename)) {
            onFileSaved(filename);
            showSuccess("File saved as: " + filename);
        } else {
            showError("Failed to save file as: " + filename);
        }
    }
}

void TerminalUI::newFile() {
    if (editor_->hasUnsavedChanges()) {
        if (!getConfirmation("Unsaved changes will be lost. Continue?")) {
            return;
        }
    }
    
    editor_->newDocument();
    onTextChanged();
    showSuccess("New document created");
}

void TerminalUI::closeFile() {
    if (editor_->hasUnsavedChanges()) {
        if (!getConfirmation("Unsaved changes will be lost. Continue?")) {
            return;
        }
    }
    
    editor_->closeDocument(editor_->getCurrentDocumentIndex());
    onTextChanged();
    showSuccess("File closed");
}

void TerminalUI::findText() {
    showFindDialog();
}

void TerminalUI::replaceText() {
    showReplaceDialog();
}

void TerminalUI::findNext() {
    if (!lastSearch_.empty()) {
        auto results = editor_->findText(lastSearch_);
        if (!results.empty()) {
            showStatus("Found " + std::to_string(results.size()) + " matches");
        }
    }
}

void TerminalUI::findPrevious() {
    // Implementation for finding previous occurrence
}

void TerminalUI::gotoLine() {
    showGotoDialog();
}

void TerminalUI::gotoPosition() {
    // Implementation for going to specific position
}

void TerminalUI::gotoBeginning() {
    if (editor_) {
        editor_->moveCursorToDocumentStart();
        onCursorMoved();
    }
}

void TerminalUI::gotoEnd() {
    if (editor_) {
        editor_->moveCursorToDocumentEnd();
        onCursorMoved();
    }
}

void TerminalUI::undo() {
    if (editor_) {
        editor_->undo();
        onTextChanged();
    }
}

void TerminalUI::redo() {
    if (editor_) {
        editor_->redo();
        onTextChanged();
    }
}

void TerminalUI::cut() {
    if (editor_) {
        editor_->cut();
        onTextChanged();
    }
}

void TerminalUI::copy() {
    if (editor_) {
        editor_->copy();
    }
}

void TerminalUI::paste() {
    if (editor_) {
        editor_->paste();
        onTextChanged();
    }
}

void TerminalUI::selectAll() {
    if (editor_) {
        editor_->selectAll();
    }
}

void TerminalUI::deleteSelection() {
    if (editor_) {
        editor_->deleteSelection();
        onTextChanged();
    }
}

void TerminalUI::toggleLineNumbers() {
    setLineNumbers(!showLineNumbers_);
    calculateDimensions();
    destroyWindows();
    createWindows();
    refreshAll();
}

void TerminalUI::toggleWordWrap() {
    setWordWrap(!wordWrap_);
}

void TerminalUI::toggleSyntaxHighlighting() {
    setSyntaxHighlighting(!syntaxHighlighting_);
}

void TerminalUI::toggleAutoComplete() {
    setAutoComplete(!autoComplete_);
}

void TerminalUI::toggleNLPFeatures() {
    setNLPFeatures(!nlpFeatures_);
}

void TerminalUI::zoomIn() {
    // Implementation for zoom in
}

void TerminalUI::zoomOut() {
    // Implementation for zoom out
}

void TerminalUI::resetZoom() {
    // Implementation for reset zoom
}

void TerminalUI::nextTheme() {
    int current = static_cast<int>(currentTheme_);
    int next = (current + 1) % 5; // 5 themes
    setTheme(static_cast<Theme>(next));
}

void TerminalUI::previousTheme() {
    int current = static_cast<int>(currentTheme_);
    int prev = (current - 1 + 5) % 5; // 5 themes
    setTheme(static_cast<Theme>(prev));
}

void TerminalUI::customizeTheme() {
    // Implementation for theme customization
}

void TerminalUI::showHelp() {
    // Implementation for help dialog
}

void TerminalUI::showAbout() {
    // Implementation for about dialog
}

void TerminalUI::showKeyboardShortcuts() {
    // Implementation for keyboard shortcuts dialog
}

std::string TerminalUI::getFileExtension(const std::string& filename) {
    size_t pos = filename.find_last_of('.');
    if (pos != std::string::npos) {
        return filename.substr(pos + 1);
    }
    return "";
}

std::string TerminalUI::getCurrentTime() {
    time_t now = time(0);
    char buffer[100];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localtime(&now));
    return std::string(buffer);
}

std::string TerminalUI::formatFileSize(size_t size) {
    const char* units[] = {"B", "KB", "MB", "GB"};
    int unit = 0;
    double fileSize = static_cast<double>(size);
    
    while (fileSize >= 1024 && unit < 3) {
        fileSize /= 1024;
        unit++;
    }
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << fileSize << " " << units[unit];
    return oss.str();
}

void TerminalUI::logMessage(const std::string& message) {
    // Log message to file or console
    std::cout << "[" << getCurrentTime() << "] " << message << std::endl;
}

void TerminalUI::onTextChanged() {
    updateSuggestions();
}

void TerminalUI::onCursorMoved() {
    updateCursorPosition();
    scrollToCursor();
}

void TerminalUI::onSelectionChanged() {
    // Handle selection changes
}

void TerminalUI::onFileOpened(const std::string& filename) {
    logMessage("File opened: " + filename);
}

void TerminalUI::onFileSaved(const std::string& filename) {
    logMessage("File saved: " + filename);
}

void TerminalUI::onError(const std::string& error) {
    logMessage("Error: " + error);
}

// Signal handlers
void setupSignalHandlers() {
    signal(SIGWINCH, handleResize);
    signal(SIGINT, handleInterrupt);
}

void handleResize(int sig) {
    if (g_ui) {
        g_ui->resizeHandler();
    }
}

void handleInterrupt(int sig) {
    if (g_ui) {
        g_ui->cleanup();
        exit(0);
    }
}

// Helper function for text wrapping
std::vector<std::string> wrapText(const std::string& text, int width) {
    std::vector<std::string> lines;
    std::istringstream iss(text);
    std::string word;
    std::string currentLine;
    
    while (iss >> word) {
        if (currentLine.length() + word.length() + 1 <= width) {
            if (!currentLine.empty()) {
                currentLine += " ";
            }
            currentLine += word;
        } else {
            if (!currentLine.empty()) {
                lines.push_back(currentLine);
                currentLine = word;
            } else {
                lines.push_back(word);
            }
        }
    }
    
    if (!currentLine.empty()) {
        lines.push_back(currentLine);
    }
    
    return lines;
}

} // namespace TextFlow
