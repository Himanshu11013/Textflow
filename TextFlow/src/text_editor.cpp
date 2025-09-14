#include "text_editor.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>
#include <thread>
#include <chrono>

namespace TextFlow {

TextEditor::TextEditor() 
    : currentDocumentIndex_(0), cursorPosition_(0), 
      selectionStart_(0), selectionEnd_(0), isSelecting_(false),
      autoSaveEnabled_(false), autoSaveInterval_(30), 
      stopAutoSave_(false) {
    newDocument();
}

bool TextEditor::newDocument() {
    documents_.push_back(std::make_unique<Document>());
    currentDocumentIndex_ = documents_.size() - 1;
    cursorPosition_ = 0;
    clearSelection();
    return true;
}

bool TextEditor::openDocument(const std::string& filename) {
    try {
        auto doc = std::make_unique<Document>(filename);
        doc->content->loadFromFile(filename);
        doc->modified = false;
        doc->lastSaved = std::chrono::system_clock::now();
        
        documents_.push_back(std::move(doc));
        currentDocumentIndex_ = documents_.size() - 1;
        cursorPosition_ = 0;
        clearSelection();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error opening file: " << e.what() << std::endl;
        return false;
    }
}

bool TextEditor::saveDocument() {
    auto* doc = getCurrentDocument();
    if (!doc) return false;
    
    try {
        doc->content->saveToFile(doc->filename);
        doc->modified = false;
        doc->lastSaved = std::chrono::system_clock::now();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error saving file: " << e.what() << std::endl;
        return false;
    }
}

bool TextEditor::saveAsDocument(const std::string& filename) {
    auto* doc = getCurrentDocument();
    if (!doc) return false;
    
    doc->filename = filename;
    doc->title = filename;
    return saveDocument();
}

bool TextEditor::closeDocument(int index) {
    if (!isValidDocumentIndex(index)) return false;
    
    documents_.erase(documents_.begin() + index);
    
    if (currentDocumentIndex_ >= static_cast<int>(documents_.size())) {
        currentDocumentIndex_ = std::max(0, static_cast<int>(documents_.size()) - 1);
    }
    
    if (documents_.empty()) {
        newDocument();
    }
    
    cursorPosition_ = 0;
    clearSelection();
    return true;
}

void TextEditor::switchToDocument(int index) {
    if (isValidDocumentIndex(index)) {
        currentDocumentIndex_ = index;
        cursorPosition_ = 0;
        clearSelection();
    }
}

void TextEditor::insertText(const std::string& text) {
    auto* doc = getCurrentDocument();
    if (!doc) return;
    
    if (hasSelection()) {
        deleteSelection();
    }
    
    doc->content->insert(cursorPosition_, text);
    cursorPosition_ += text.length();
    markDocumentModified();
}

void TextEditor::deleteText(int length) {
    auto* doc = getCurrentDocument();
    if (!doc) return;
    
    if (hasSelection()) {
        deleteSelection();
        return;
    }
    
    int actualLength = std::min(length, doc->content->getSize() - cursorPosition_);
    if (actualLength > 0) {
        doc->content->erase(cursorPosition_, actualLength);
        markDocumentModified();
    }
}

void TextEditor::backspace(int length) {
    auto* doc = getCurrentDocument();
    if (!doc) return;
    
    if (hasSelection()) {
        deleteSelection();
        return;
    }
    
    int actualLength = std::min(length, cursorPosition_);
    if (actualLength > 0) {
        cursorPosition_ -= actualLength;
        doc->content->erase(cursorPosition_, actualLength);
        markDocumentModified();
    }
}

void TextEditor::insertNewline() {
    insertText("\n");
}

void TextEditor::insertTab() {
    insertText("    "); // 4 spaces for tab
}

void TextEditor::moveCursor(int offset) {
    auto* doc = getCurrentDocument();
    if (!doc) return;
    
    int newPosition = doc->content->moveCursor(cursorPosition_, offset);
    cursorPosition_ = newPosition;
    ensureValidCursorPosition();
}

void TextEditor::moveCursorTo(int position) {
    auto* doc = getCurrentDocument();
    if (!doc) return;
    
    cursorPosition_ = std::max(0, std::min(position, doc->content->getSize()));
    ensureValidCursorPosition();
}

void TextEditor::moveCursorToLine(int line) {
    auto* doc = getCurrentDocument();
    if (!doc) return;
    
    int currentLine = getCurrentLine();
    int lineDiff = line - currentLine;
    
    if (lineDiff > 0) {
        for (int i = 0; i < lineDiff; ++i) {
            moveCursorDown();
        }
    } else if (lineDiff < 0) {
        for (int i = 0; i < -lineDiff; ++i) {
            moveCursorUp();
        }
    }
}

void TextEditor::moveCursorToLineStart() {
    auto* doc = getCurrentDocument();
    if (!doc) return;
    
    cursorPosition_ = doc->content->getLineStart(cursorPosition_);
}

void TextEditor::moveCursorToLineEnd() {
    auto* doc = getCurrentDocument();
    if (!doc) return;
    
    cursorPosition_ = doc->content->getLineEnd(cursorPosition_);
}

void TextEditor::moveCursorToDocumentStart() {
    cursorPosition_ = 0;
}

void TextEditor::moveCursorToDocumentEnd() {
    auto* doc = getCurrentDocument();
    if (doc) {
        cursorPosition_ = doc->content->getSize();
    }
}

void TextEditor::moveCursorUp() {
    auto* doc = getCurrentDocument();
    if (!doc) return;
    
    int currentLine = getCurrentLine();
    int currentColumn = getCurrentColumn();
    
    if (currentLine > 1) {
        moveCursorToLine(currentLine - 1);
        int newLineStart = doc->content->getLineStart(cursorPosition_);
        int newLineEnd = doc->content->getLineEnd(cursorPosition_);
        int maxColumn = newLineEnd - newLineStart;
        moveCursorTo(newLineStart + std::min(currentColumn, maxColumn));
    }
}

void TextEditor::moveCursorDown() {
    auto* doc = getCurrentDocument();
    if (!doc) return;
    
    int currentLine = getCurrentLine();
    int currentColumn = getCurrentColumn();
    int totalLines = getLineCount();
    
    if (currentLine < totalLines) {
        moveCursorToLine(currentLine + 1);
        int newLineStart = doc->content->getLineStart(cursorPosition_);
        int newLineEnd = doc->content->getLineEnd(cursorPosition_);
        int maxColumn = newLineEnd - newLineStart;
        moveCursorTo(newLineStart + std::min(currentColumn, maxColumn));
    }
}

void TextEditor::moveCursorLeft() {
    if (cursorPosition_ > 0) {
        moveCursor(-1);
    }
}

void TextEditor::moveCursorRight() {
    auto* doc = getCurrentDocument();
    if (doc && cursorPosition_ < doc->content->getSize()) {
        moveCursor(1);
    }
}

void TextEditor::startSelection() {
    isSelecting_ = true;
    selectionStart_ = cursorPosition_;
    selectionEnd_ = cursorPosition_;
}

void TextEditor::endSelection() {
    if (isSelecting_) {
        selectionEnd_ = cursorPosition_;
        isSelecting_ = false;
        normalizeSelection();
    }
}

void TextEditor::clearSelection() {
    selectionStart_ = selectionEnd_ = cursorPosition_;
    isSelecting_ = false;
}


void TextEditor::selectLine() {
    auto* doc = getCurrentDocument();
    if (!doc) return;
    
    selectionStart_ = doc->content->getLineStart(cursorPosition_);
    selectionEnd_ = doc->content->getLineEnd(cursorPosition_);
    isSelecting_ = false;
}

void TextEditor::selectWord() {
    auto* doc = getCurrentDocument();
    if (!doc) return;
    
    // Find word boundaries
    int start = cursorPosition_;
    int end = cursorPosition_;
    
    // Move start backwards to word beginning
    while (start > 0) {
        char c = doc->content->getChar(start - 1);
        if (!std::isalnum(c) && c != '_') break;
        start--;
    }
    
    // Move end forwards to word end
    while (end < doc->content->getSize()) {
        char c = doc->content->getChar(end);
        if (!std::isalnum(c) && c != '_') break;
        end++;
    }
    
    selectionStart_ = start;
    selectionEnd_ = end;
    isSelecting_ = false;
}

void TextEditor::copy() {
    if (hasSelection()) {
        clipboard_ = getSelectedText();
    }
}

void TextEditor::cut() {
    copy();
    deleteSelection();
}

void TextEditor::paste() {
    if (!clipboard_.empty()) {
        insertText(clipboard_);
    }
}

void TextEditor::undo() {
    auto* doc = getCurrentDocument();
    if (doc && doc->content->undo()) {
        markDocumentModified();
        ensureValidCursorPosition();
    }
}

void TextEditor::redo() {
    auto* doc = getCurrentDocument();
    if (doc && doc->content->redo()) {
        markDocumentModified();
        ensureValidCursorPosition();
    }
}

void TextEditor::deleteSelection() {
    if (!hasSelection()) return;
    
    auto* doc = getCurrentDocument();
    if (!doc) return;
    
    int length = selectionEnd_ - selectionStart_;
    doc->content->erase(selectionStart_, length);
    cursorPosition_ = selectionStart_;
    clearSelection();
    markDocumentModified();
}

std::vector<int> TextEditor::findText(const std::string& pattern, bool caseSensitive) {
    auto* doc = getCurrentDocument();
    if (!doc) return {};
    
    std::string searchPattern = pattern;
    if (!caseSensitive) {
        std::transform(searchPattern.begin(), searchPattern.end(), searchPattern.begin(), ::tolower);
    }
    
    std::vector<int> results = doc->content->findAll(pattern);
    
    if (!caseSensitive) {
        // Filter results for case-insensitive matches
        std::vector<int> filteredResults;
        for (int pos : results) {
            std::string text = doc->content->getText(pos, pattern.length());
            std::transform(text.begin(), text.end(), text.begin(), ::tolower);
            if (text == searchPattern) {
                filteredResults.push_back(pos);
            }
        }
        return filteredResults;
    }
    
    return results;
}

std::vector<int> TextEditor::findRegex(const std::string& pattern) {
    auto* doc = getCurrentDocument();
    if (!doc) return {};
    
    return doc->content->findAllRegex(pattern);
}

void TextEditor::replaceText(const std::string& pattern, const std::string& replacement, bool all) {
    auto* doc = getCurrentDocument();
    if (!doc) return;
    
    std::vector<int> positions = findText(pattern);
    
    if (all) {
        // Replace all occurrences
        for (auto it = positions.rbegin(); it != positions.rend(); ++it) {
            doc->content->erase(*it, pattern.length());
            doc->content->insert(*it, replacement);
        }
    } else if (!positions.empty()) {
        // Replace first occurrence
        int pos = positions[0];
        doc->content->erase(pos, pattern.length());
        doc->content->insert(pos, replacement);
    }
    
    markDocumentModified();
}

void TextEditor::replaceRegex(const std::string& pattern, const std::string& replacement, bool all) {
    auto* doc = getCurrentDocument();
    if (!doc) return;
    
    std::vector<int> positions = findRegex(pattern);
    
    if (all) {
        // Replace all occurrences
        for (auto it = positions.rbegin(); it != positions.rend(); ++it) {
            // For regex, we need to get the actual matched text length
            // This is a simplified implementation
            doc->content->erase(*it, pattern.length());
            doc->content->insert(*it, replacement);
        }
    } else if (!positions.empty()) {
        // Replace first occurrence
        int pos = positions[0];
        doc->content->erase(pos, pattern.length());
        doc->content->insert(pos, replacement);
    }
    
    markDocumentModified();
}

void TextEditor::enableAutoSave(int intervalSeconds) {
    autoSaveEnabled_ = true;
    autoSaveInterval_ = intervalSeconds;
    stopAutoSave_ = false;
    
    if (autoSaveThread_.joinable()) {
        autoSaveThread_.join();
    }
    
    autoSaveThread_ = std::thread(&TextEditor::performAutoSaveLoop, this);
}

void TextEditor::disableAutoSave() {
    stopAutoSave_ = true;
    autoSaveEnabled_ = false;
    
    if (autoSaveThread_.joinable()) {
        autoSaveThread_.join();
    }
}

void TextEditor::performAutoSave() {
    auto* doc = getCurrentDocument();
    if (doc && doc->modified) {
        std::string backupFile = doc->filename + ".autosave";
        try {
            doc->content->saveToFile(backupFile);
            lastAutoSave_ = std::chrono::system_clock::now();
        } catch (const std::exception& e) {
            std::cerr << "Auto-save failed: " << e.what() << std::endl;
        }
    }
}

bool TextEditor::hasUnsavedChanges() const {
    for (const auto& doc : documents_) {
        if (doc && doc->modified) {
            return true;
        }
    }
    return false;
}

Document* TextEditor::getCurrentDocument() const {
    if (isValidDocumentIndex(currentDocumentIndex_)) {
        return documents_[currentDocumentIndex_].get();
    }
    return nullptr;
}

Document* TextEditor::getDocument(int index) const {
    if (isValidDocumentIndex(index)) {
        return documents_[index].get();
    }
    return nullptr;
}

std::string TextEditor::getText() const {
    auto* doc = getCurrentDocument();
    if (!doc) return "";
    
    return doc->content->getText(0, doc->content->getSize());
}

std::string TextEditor::getText(int start, int length) const {
    auto* doc = getCurrentDocument();
    if (!doc) return "";
    
    return doc->content->getText(start, length);
}

std::string TextEditor::getSelectedText() const {
    if (!hasSelection()) return "";
    
    return getText(selectionStart_, selectionEnd_ - selectionStart_);
}

std::string TextEditor::getLine(int lineNumber) const {
    auto* doc = getCurrentDocument();
    if (!doc) return "";
    
    // This is a simplified implementation
    std::string text = getText();
    std::istringstream iss(text);
    std::string line;
    
    for (int i = 1; i <= lineNumber; ++i) {
        if (!std::getline(iss, line)) {
            return "";
        }
    }
    
    return line;
}

int TextEditor::getLineCount() const {
    auto* doc = getCurrentDocument();
    if (!doc) return 0;
    
    std::string text = getText();
    return std::count(text.begin(), text.end(), '\n') + 1;
}

int TextEditor::getCurrentLine() const {
    auto* doc = getCurrentDocument();
    if (!doc) return 1;
    
    return doc->content->getLineNumber(cursorPosition_);
}

int TextEditor::getCurrentColumn() const {
    auto* doc = getCurrentDocument();
    if (!doc) return 1;
    
    return doc->content->getColumnNumber(cursorPosition_) + 1;
}

std::string TextEditor::getStatusText() const {
    auto* doc = getCurrentDocument();
    if (!doc) return "No document";
    
    std::ostringstream oss;
    oss << doc->title;
    if (doc->modified) oss << " *";
    oss << " | Line " << getCurrentLine() << ", Col " << getCurrentColumn();
    oss << " | " << doc->content->getSize() << " chars";
    
    return oss.str();
}

bool TextEditor::isModified() const {
    auto* doc = getCurrentDocument();
    return doc ? doc->modified : false;
}

bool TextEditor::loadCompressedFile(const std::string& filename) {
    // This will be implemented with compression module
    return openDocument(filename);
}

bool TextEditor::saveCompressedFile(const std::string& filename) {
    // This will be implemented with compression module
    return saveAsDocument(filename);
}

bool TextEditor::loadEncryptedFile(const std::string& filename, const std::string& password) {
    // This will be implemented with encryption module
    return openDocument(filename);
}

bool TextEditor::saveEncryptedFile(const std::string& filename, const std::string& password) {
    // This will be implemented with encryption module
    return saveAsDocument(filename);
}

// Private helper methods

void TextEditor::updateCursorPosition() {
    ensureValidCursorPosition();
}

void TextEditor::normalizeSelection() {
    if (selectionStart_ > selectionEnd_) {
        std::swap(selectionStart_, selectionEnd_);
    }
}

void TextEditor::performAutoSaveLoop() {
    while (!stopAutoSave_ && autoSaveEnabled_) {
        std::this_thread::sleep_for(std::chrono::seconds(autoSaveInterval_));
        if (!stopAutoSave_) {
            performAutoSave();
        }
    }
}

void TextEditor::markDocumentModified() {
    auto* doc = getCurrentDocument();
    if (doc) {
        doc->modified = true;
    }
}

bool TextEditor::isValidDocumentIndex(int index) const {
    return index >= 0 && index < static_cast<int>(documents_.size());
}

void TextEditor::ensureValidCursorPosition() {
    auto* doc = getCurrentDocument();
    if (doc) {
        cursorPosition_ = std::max(0, std::min(cursorPosition_, doc->content->getSize()));
    }
}

} // namespace TextFlow
