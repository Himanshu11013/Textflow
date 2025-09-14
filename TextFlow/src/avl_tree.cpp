#include "avl_tree.h"
#include <fstream>
#include <sstream>
#include <regex>
#include <algorithm>
#include <iostream>

namespace TextFlow {

AVLTree::AVLTree() : root_(nullptr) {}

void AVLTree::insert(int position, const std::string& text) {
    if (text.empty()) return;
    
    saveState();
    root_ = insertHelper(root_, position, text);
}

void AVLTree::erase(int position, int length) {
    if (length <= 0) return;
    
    saveState();
    root_ = eraseHelper(root_, position, length);
}

std::string AVLTree::getText(int start, int length) const {
    std::string result;
    int currentPos = 0;
    collectText(root_, start, length, result, currentPos);
    return result;
}

char AVLTree::getChar(int position) const {
    if (position < 0 || position >= getSize()) return '\0';
    
    std::string text = getText(position, 1);
    return text.empty() ? '\0' : text[0];
}

int AVLTree::getSize() const {
    return getSize(root_);
}

int AVLTree::moveCursor(int currentPos, int offset) const {
    int newPos = currentPos + offset;
    int totalSize = getSize();
    return std::max(0, std::min(newPos, totalSize));
}

int AVLTree::getLineStart(int position) const {
    std::string text = getText(0, position);
    size_t lastNewline = text.rfind('\n');
    return lastNewline == std::string::npos ? 0 : lastNewline + 1;
}

int AVLTree::getLineEnd(int position) const {
    std::string text = getText(position, getSize() - position);
    size_t nextNewline = text.find('\n');
    return nextNewline == std::string::npos ? getSize() : position + nextNewline;
}

int AVLTree::getLineNumber(int position) const {
    std::string text = getText(0, position);
    return std::count(text.begin(), text.end(), '\n') + 1;
}

int AVLTree::getColumnNumber(int position) const {
    int lineStart = getLineStart(position);
    return position - lineStart;
}

void AVLTree::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    
    clearHistory();
    root_ = nullptr;
    if (!content.empty()) {
        insert(0, content);
    }
}

void AVLTree::saveToFile(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot create file: " + filename);
    }
    
    inOrderTraversal(root_, [&file](const std::string& text) {
        file << text;
    });
}

void AVLTree::saveState() {
    if (root_) {
        undoStack_.push(root_);
        if (undoStack_.size() > 50) { // Limit history size
            std::stack<std::shared_ptr<AVLNode>> temp;
            while (undoStack_.size() > 25) {
                temp.push(undoStack_.top());
                undoStack_.pop();
            }
            undoStack_ = temp;
        }
    }
    redoStack_ = std::stack<std::shared_ptr<AVLNode>>(); // Clear redo stack
}

bool AVLTree::undo() {
    if (undoStack_.empty()) return false;
    
    redoStack_.push(root_);
    root_ = undoStack_.top();
    undoStack_.pop();
    return true;
}

bool AVLTree::redo() {
    if (redoStack_.empty()) return false;
    
    undoStack_.push(root_);
    root_ = redoStack_.top();
    redoStack_.pop();
    return true;
}

void AVLTree::clearHistory() {
    undoStack_ = std::stack<std::shared_ptr<AVLNode>>();
    redoStack_ = std::stack<std::shared_ptr<AVLNode>>();
}

std::vector<int> AVLTree::findAll(const std::string& pattern) const {
    std::vector<int> results;
    int currentPos = 0;
    searchInNode(root_, pattern, results, currentPos);
    return results;
}

std::vector<int> AVLTree::findAllRegex(const std::string& pattern) const {
    std::vector<int> results;
    int currentPos = 0;
    searchRegexInNode(root_, pattern, results, currentPos);
    return results;
}

void AVLTree::printTree() const {
    std::function<void(std::shared_ptr<AVLNode>, int)> printHelper = 
        [&printHelper](std::shared_ptr<AVLNode> node, int depth) {
            if (!node) return;
            
            printHelper(node->right, depth + 1);
            for (int i = 0; i < depth; ++i) std::cout << "  ";
            std::cout << "[" << node->data << "] (h:" << node->height << ", s:" << node->size << ")\n";
            printHelper(node->left, depth + 1);
        };
    
    printHelper(root_, 0);
}

bool AVLTree::validate() const {
    std::function<bool(std::shared_ptr<AVLNode>)> validateHelper = 
        [this, &validateHelper](std::shared_ptr<AVLNode> node) -> bool {
            if (!node) return true;
            
            int balance = getBalance(node);
            if (balance < -1 || balance > 1) return false;
            
            int expectedSize = node->data.length();
            if (node->left) expectedSize += node->left->size;
            if (node->right) expectedSize += node->right->size;
            
            if (node->size != expectedSize) return false;
            
            return validateHelper(node->left) && validateHelper(node->right);
        };
    
    return validateHelper(root_);
}

// Private helper methods

int AVLTree::getHeight(std::shared_ptr<AVLNode> node) const {
    return node ? node->height : 0;
}

int AVLTree::getSize(std::shared_ptr<AVLNode> node) const {
    return node ? node->size : 0;
}

int AVLTree::getBalance(std::shared_ptr<AVLNode> node) const {
    return node ? getHeight(node->left) - getHeight(node->right) : 0;
}

void AVLTree::updateHeight(std::shared_ptr<AVLNode> node) {
    if (node) {
        node->height = 1 + std::max(getHeight(node->left), getHeight(node->right));
    }
}

void AVLTree::updateSize(std::shared_ptr<AVLNode> node) {
    if (node) {
        node->size = node->data.length() + getSize(node->left) + getSize(node->right);
    }
}

std::shared_ptr<AVLNode> AVLTree::rotateRight(std::shared_ptr<AVLNode> y) {
    auto x = y->left;
    auto T2 = x->right;
    
    x->right = y;
    y->left = T2;
    
    updateHeight(y);
    updateSize(y);
    updateHeight(x);
    updateSize(x);
    
    return x;
}

std::shared_ptr<AVLNode> AVLTree::rotateLeft(std::shared_ptr<AVLNode> x) {
    auto y = x->right;
    auto T2 = y->left;
    
    y->left = x;
    x->right = T2;
    
    updateHeight(x);
    updateSize(x);
    updateHeight(y);
    updateSize(y);
    
    return y;
}

std::shared_ptr<AVLNode> AVLTree::balance(std::shared_ptr<AVLNode> node) {
    if (!node) return node;
    
    updateHeight(node);
    updateSize(node);
    
    int balance = getBalance(node);
    
    // Left Left Case
    if (balance > 1 && getBalance(node->left) >= 0) {
        return rotateRight(node);
    }
    
    // Right Right Case
    if (balance < -1 && getBalance(node->right) <= 0) {
        return rotateLeft(node);
    }
    
    // Left Right Case
    if (balance > 1 && getBalance(node->left) < 0) {
        node->left = rotateLeft(node->left);
        return rotateRight(node);
    }
    
    // Right Left Case
    if (balance < -1 && getBalance(node->right) > 0) {
        node->right = rotateRight(node->right);
        return rotateLeft(node);
    }
    
    return node;
}

std::shared_ptr<AVLNode> AVLTree::insertHelper(std::shared_ptr<AVLNode> node, int position, const std::string& text) {
    if (!node) {
        return std::make_shared<AVLNode>(text);
    }
    
    int leftSize = getSize(node->left);
    
    if (position <= leftSize) {
        node->left = insertHelper(node->left, position, text);
    } else if (position <= leftSize + static_cast<int>(node->data.length())) {
        // Insert within current node's data
        int insertPos = position - leftSize;
        std::string leftPart = node->data.substr(0, insertPos);
        std::string rightPart = node->data.substr(insertPos);
        
        node->data = leftPart + text + rightPart;
        updateSize(node);
    } else {
        node->right = insertHelper(node->right, position - leftSize - static_cast<int>(node->data.length()), text);
    }
    
    return balance(node);
}

std::shared_ptr<AVLNode> AVLTree::eraseHelper(std::shared_ptr<AVLNode> node, int position, int length) {
    if (!node) return node;
    
    int leftSize = getSize(node->left);
    int nodeDataLength = static_cast<int>(node->data.length());
    
    if (position < leftSize) {
        // Erase from left subtree
        node->left = eraseHelper(node->left, position, length);
    } else if (position < leftSize + nodeDataLength) {
        // Erase from current node
        int eraseStart = position - leftSize;
        int eraseEnd = std::min(eraseStart + length, nodeDataLength);
        
        std::string leftPart = node->data.substr(0, eraseStart);
        std::string rightPart = node->data.substr(eraseEnd);
        
        node->data = leftPart + rightPart;
        updateSize(node);
        
        // If there's remaining length to erase, continue with right subtree
        int remainingLength = length - (eraseEnd - eraseStart);
        if (remainingLength > 0) {
            node->right = eraseHelper(node->right, 0, remainingLength);
        }
    } else {
        // Erase from right subtree
        node->right = eraseHelper(node->right, position - leftSize - nodeDataLength, length);
    }
    
    return balance(node);
}

std::shared_ptr<AVLNode> AVLTree::findMin(std::shared_ptr<AVLNode> node) const {
    while (node && node->left) {
        node = node->left;
    }
    return node;
}

std::shared_ptr<AVLNode> AVLTree::findMax(std::shared_ptr<AVLNode> node) const {
    while (node && node->right) {
        node = node->right;
    }
    return node;
}

void AVLTree::inOrderTraversal(std::shared_ptr<AVLNode> node, std::function<void(const std::string&)> callback) const {
    if (!node) return;
    
    inOrderTraversal(node->left, callback);
    callback(node->data);
    inOrderTraversal(node->right, callback);
}

void AVLTree::collectText(std::shared_ptr<AVLNode> node, int start, int length, std::string& result, int& currentPos) const {
    if (!node) return;
    
    collectText(node->left, start, length, result, currentPos);
    
    // Check if we need to include this node's data
    if (currentPos < start + length && currentPos + static_cast<int>(node->data.length()) > start) {
        int nodeStart = std::max(0, start - currentPos);
        int nodeEnd = std::min(static_cast<int>(node->data.length()), start + length - currentPos);
        
        if (nodeStart < nodeEnd) {
            result += node->data.substr(nodeStart, nodeEnd - nodeStart);
        }
    }
    
    currentPos += node->data.length();
    collectText(node->right, start, length, result, currentPos);
}

void AVLTree::searchInNode(std::shared_ptr<AVLNode> node, const std::string& pattern, std::vector<int>& results, int& currentPos) const {
    if (!node) return;
    
    searchInNode(node->left, pattern, results, currentPos);
    
    // Search within current node's data
    size_t pos = 0;
    while ((pos = node->data.find(pattern, pos)) != std::string::npos) {
        results.push_back(currentPos + static_cast<int>(pos));
        pos += pattern.length();
    }
    
    currentPos += node->data.length();
    searchInNode(node->right, pattern, results, currentPos);
}

void AVLTree::searchRegexInNode(std::shared_ptr<AVLNode> node, const std::string& pattern, std::vector<int>& results, int& currentPos) const {
    if (!node) return;
    
    searchRegexInNode(node->left, pattern, results, currentPos);
    
    try {
        std::regex regexPattern(pattern);
        std::sregex_iterator iter(node->data.begin(), node->data.end(), regexPattern);
        std::sregex_iterator end;
        
        for (; iter != end; ++iter) {
            results.push_back(currentPos + static_cast<int>(iter->position()));
        }
    } catch (const std::regex_error& e) {
        // Invalid regex pattern, skip
    }
    
    currentPos += node->data.length();
    searchRegexInNode(node->right, pattern, results, currentPos);
}

} // namespace TextFlow
