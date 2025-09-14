#pragma once

#include "avl_tree.h"
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <thread>

namespace TextFlow {

struct Document {
    std::string filename;
    std::string title;
    bool modified;
    std::chrono::system_clock::time_point lastSaved;
    std::unique_ptr<AVLTree> content;
    
    Document(const std::string& name = "Untitled") 
        : filename(name), title(name), modified(false), 
          lastSaved(std::chrono::system_clock::now()),
          content(std::make_unique<AVLTree>()) {}
};

class TextEditor {
public:
    TextEditor();
    ~TextEditor() = default;
    
    // Document management
    bool newDocument();
    bool openDocument(const std::string& filename);
    bool saveDocument();
    bool saveAsDocument(const std::string& filename);
    bool closeDocument(int index);
    void switchToDocument(int index);
    
    // Current document operations
    void insertText(const std::string& text);
    void deleteText(int length);
    void backspace(int length);
    void insertNewline();
    void insertTab();
    
    // Cursor operations
    void moveCursor(int offset);
    void moveCursorTo(int position);
    void moveCursorToLine(int line);
    void moveCursorToLineStart();
    void moveCursorToLineEnd();
    void moveCursorToDocumentStart();
    void moveCursorToDocumentEnd();
    void moveCursorUp();
    void moveCursorDown();
    void moveCursorLeft();
    void moveCursorRight();
    
    // Selection operations
    void startSelection();
    void endSelection();
    void clearSelection();
    void selectLine();
    void selectWord();
    
    // Text operations
    void copy();
    void cut();
    void paste();
    void undo();
    void redo();
    void deleteSelection();
    
    // Search and replace
    std::vector<int> findText(const std::string& pattern, bool caseSensitive = false);
    std::vector<int> findRegex(const std::string& pattern);
    void replaceText(const std::string& pattern, const std::string& replacement, bool all = false);
    void replaceRegex(const std::string& pattern, const std::string& replacement, bool all = false);
    
    // Auto-save and recovery
    void enableAutoSave(int intervalSeconds = 30);
    void disableAutoSave();
    void performAutoSave();
    bool hasUnsavedChanges() const;
    
    // Getters
    int getCurrentDocumentIndex() const { return currentDocumentIndex_; }
    Document* getCurrentDocument() const;
    Document* getDocument(int index) const;
    int getDocumentCount() const { return documents_.size(); }
    int getCursorPosition() const { return cursorPosition_; }
    int getSelectionStart() const { return selectionStart_; }
    int getSelectionEnd() const { return selectionEnd_; }
    bool hasSelection() const { return selectionStart_ != selectionEnd_; }
    
    // Text content
    std::string getText() const;
    std::string getText(int start, int length) const;
    std::string getSelectedText() const;
    std::string getLine(int lineNumber) const;
    int getLineCount() const;
    int getCurrentLine() const;
    int getCurrentColumn() const;
    
    // Status
    std::string getStatusText() const;
    bool isModified() const;
    
    // File operations with compression/encryption
    bool loadCompressedFile(const std::string& filename);
    bool saveCompressedFile(const std::string& filename);
    bool loadEncryptedFile(const std::string& filename, const std::string& password);
    bool saveEncryptedFile(const std::string& filename, const std::string& password);

private:
    std::vector<std::unique_ptr<Document>> documents_;
    int currentDocumentIndex_;
    int cursorPosition_;
    int selectionStart_;
    int selectionEnd_;
    bool isSelecting_;
    
    // Auto-save
    bool autoSaveEnabled_;
    int autoSaveInterval_;
    std::chrono::system_clock::time_point lastAutoSave_;
    std::thread autoSaveThread_;
    bool stopAutoSave_;
    
    // Clipboard
    std::string clipboard_;
    
    // Helper methods
    void updateCursorPosition();
    void normalizeSelection();
    void performAutoSaveLoop();
    void markDocumentModified();
    bool isValidDocumentIndex(int index) const;
    void ensureValidCursorPosition();
};

} // namespace TextFlow
