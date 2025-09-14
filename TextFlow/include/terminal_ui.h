#pragma once

#include "text_editor.h"
#include <ncurses.h>
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <functional>

namespace TextFlow {

// Color themes
enum class Theme {
    DARK,
    LIGHT,
    MONOKAI,
    SOLARIZED_DARK,
    SOLARIZED_LIGHT
};

// UI components
struct ColorScheme {
    int background;
    int foreground;
    int cursor;
    int selection;
    int status_bar;
    int menu_bar;
    int error;
    int warning;
    int success;
    int highlight;
};

class TerminalUI {
public:
    TerminalUI();
    ~TerminalUI();
    
    // Initialization and cleanup
    bool initialize();
    void cleanup();
    void run();
    
    // Theme management
    void setTheme(Theme theme);
    void loadTheme(const std::string& themeName);
    void saveTheme(const std::string& themeName);
    
    // Window management
    void createWindows();
    void destroyWindows();
    void refreshAll();
    void resizeHandler();
    
    // Main UI loop
    void mainLoop();
    void handleInput();
    void updateDisplay();
    
    // Text editor integration
    void setTextEditor(std::shared_ptr<TextEditor> editor);
    
    // Display functions
    void displayText();
    void displayStatusBar();
    void displayMenuBar();
    void displayLineNumbers();
    void displayCursor();
    void displaySelection();
    
    // Input handling
    void handleKeyPress(int key);
    void handleSpecialKeys(int key);
    void handleMouse(int x, int y);
    
    // Dialog boxes
    void showOpenDialog();
    void showSaveDialog();
    void showFindDialog();
    void showReplaceDialog();
    void showGotoDialog();
    void showThemeDialog();
    void showNLPDialog();
    
    // Status and notifications
    void showStatus(const std::string& message, int timeout = 3);
    void showError(const std::string& error);
    void showWarning(const std::string& warning);
    void showSuccess(const std::string& message);
    
    // Configuration
    void setLineNumbers(bool enabled);
    void setWordWrap(bool enabled);
    void setSyntaxHighlighting(bool enabled);
    void setAutoComplete(bool enabled);
    void setNLPFeatures(bool enabled);
    
    // Getters
    bool isRunning() const { return running_; }
    Theme getCurrentTheme() const { return currentTheme_; }
    bool getLineNumbers() const { return showLineNumbers_; }
    bool getWordWrap() const { return wordWrap_; }
    bool getSyntaxHighlighting() const { return syntaxHighlighting_; }
    bool getAutoComplete() const { return autoComplete_; }
    bool getNLPFeatures() const { return nlpFeatures_; }

private:
    // Core components
    std::shared_ptr<TextEditor> editor_;
    
    // UI state
    bool running_;
    bool initialized_;
    int screenHeight_;
    int screenWidth_;
    int textAreaHeight_;
    int textAreaWidth_;
    int textStartX_;
    int textStartY_;
    
    // Windows
    WINDOW* mainWindow_;
    WINDOW* textWindow_;
    WINDOW* statusWindow_;
    WINDOW* menuWindow_;
    WINDOW* lineNumberWindow_;
    
    // Display state
    int scrollX_;
    int scrollY_;
    int cursorX_;
    int cursorY_;
    bool showLineNumbers_;
    bool wordWrap_;
    bool syntaxHighlighting_;
    bool autoComplete_;
    bool nlpFeatures_;
    
    // Theme
    Theme currentTheme_;
    ColorScheme colors_;
    std::map<Theme, ColorScheme> themes_;
    
    // Input state
    std::string inputBuffer_;
    bool inInputMode_;
    std::string lastSearch_;
    std::string lastReplace_;
    
    // NLP integration
    std::vector<std::string> suggestions_;
    int suggestionIndex_;
    bool showSuggestions_;
    
    // Helper functions
    void initializeThemes();
    void setupColors();
    void calculateDimensions();
    void updateCursorPosition();
    void scrollToCursor();
    void scrollToPosition(int x, int y);
    
    // Text rendering
    void renderTextLine(int line, int startCol, int endCol);
    void renderLineNumbers();
    void highlightSyntax(const std::string& text, int y, int x);
    void highlightSelection(int startX, int startY, int endX, int endY);
    
    // Input processing
    std::string getInput(const std::string& prompt, const std::string& defaultValue = "");
    int getChoice(const std::string& prompt, const std::vector<std::string>& options);
    bool getConfirmation(const std::string& message);
    
    // Dialog implementations
    void drawDialog(int x, int y, int width, int height, const std::string& title);
    void drawInputField(int x, int y, int width, const std::string& text, bool selected);
    void drawButton(int x, int y, int width, const std::string& text, bool selected);
    
    // NLP features
    void updateSuggestions();
    void displaySuggestions();
    void applySuggestion(int index);
    void processTextWithNLP();
    
    // File operations
    void openFile();
    void saveFile();
    void saveAsFile();
    void newFile();
    void closeFile();
    
    // Search and replace
    void findText();
    void replaceText();
    void findNext();
    void findPrevious();
    
    // Navigation
    void gotoLine();
    void gotoPosition();
    void gotoBeginning();
    void gotoEnd();
    
    // Editing operations
    void undo();
    void redo();
    void cut();
    void copy();
    void paste();
    void selectAll();
    void deleteSelection();
    
    // View operations
    void toggleLineNumbers();
    void toggleWordWrap();
    void toggleSyntaxHighlighting();
    void toggleAutoComplete();
    void toggleNLPFeatures();
    void zoomIn();
    void zoomOut();
    void resetZoom();
    
    // Theme operations
    void nextTheme();
    void previousTheme();
    void customizeTheme();
    
    // Help and about
    void showHelp();
    void showAbout();
    void showKeyboardShortcuts();
    
    // Utility functions
    std::string getFileExtension(const std::string& filename);
    std::string getCurrentTime();
    std::string formatFileSize(size_t size);
    void logMessage(const std::string& message);
    
    // Event handlers
    void onTextChanged();
    void onCursorMoved();
    void onSelectionChanged();
    void onFileOpened(const std::string& filename);
    void onFileSaved(const std::string& filename);
    void onError(const std::string& error);
};

// Global UI instance
extern std::unique_ptr<TerminalUI> g_ui;

// Signal handlers
void setupSignalHandlers();
void handleResize(int sig);
void handleInterrupt(int sig);

} // namespace TextFlow
