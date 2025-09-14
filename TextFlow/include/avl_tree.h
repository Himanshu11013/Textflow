#pragma once

#include <memory>
#include <string>
#include <stack>
#include <functional>

namespace TextFlow {

struct AVLNode {
    std::string data;
    int height;
    int size;  // Total character count in subtree
    std::shared_ptr<AVLNode> left;
    std::shared_ptr<AVLNode> right;
    
    AVLNode(const std::string& d) : data(d), height(1), size(d.length()), left(nullptr), right(nullptr) {}
};

class AVLTree {
public:
    AVLTree();
    ~AVLTree() = default;
    
    // Core operations
    void insert(int position, const std::string& text);
    void erase(int position, int length);
    std::string getText(int start, int length) const;
    char getChar(int position) const;
    int getSize() const;
    
    // Cursor operations
    int moveCursor(int currentPos, int offset) const;
    int getLineStart(int position) const;
    int getLineEnd(int position) const;
    int getLineNumber(int position) const;
    int getColumnNumber(int position) const;
    
    // File operations
    void loadFromFile(const std::string& filename);
    void saveToFile(const std::string& filename) const;
    
    // Undo/Redo
    void saveState();
    bool undo();
    bool redo();
    void clearHistory();
    
    // Search operations
    std::vector<int> findAll(const std::string& pattern) const;
    std::vector<int> findAllRegex(const std::string& pattern) const;
    
    // Debugging
    void printTree() const;
    bool validate() const;

private:
    std::shared_ptr<AVLNode> root_;
    std::stack<std::shared_ptr<AVLNode>> undoStack_;
    std::stack<std::shared_ptr<AVLNode>> redoStack_;
    
    // AVL tree operations
    int getHeight(std::shared_ptr<AVLNode> node) const;
    int getSize(std::shared_ptr<AVLNode> node) const;
    int getBalance(std::shared_ptr<AVLNode> node) const;
    void updateHeight(std::shared_ptr<AVLNode> node);
    void updateSize(std::shared_ptr<AVLNode> node);
    
    std::shared_ptr<AVLNode> rotateRight(std::shared_ptr<AVLNode> y);
    std::shared_ptr<AVLNode> rotateLeft(std::shared_ptr<AVLNode> x);
    std::shared_ptr<AVLNode> balance(std::shared_ptr<AVLNode> node);
    
    std::shared_ptr<AVLNode> insertHelper(std::shared_ptr<AVLNode> node, int position, const std::string& text);
    std::shared_ptr<AVLNode> eraseHelper(std::shared_ptr<AVLNode> node, int position, int length);
    
    // Utility functions
    std::shared_ptr<AVLNode> findMin(std::shared_ptr<AVLNode> node) const;
    std::shared_ptr<AVLNode> findMax(std::shared_ptr<AVLNode> node) const;
    std::shared_ptr<AVLNode> findNodeAtPosition(std::shared_ptr<AVLNode> node, int position) const;
    
    // Traversal helpers
    void inOrderTraversal(std::shared_ptr<AVLNode> node, std::function<void(const std::string&)> callback) const;
    void collectText(std::shared_ptr<AVLNode> node, int start, int length, std::string& result, int& currentPos) const;
    
    // Search helpers
    void searchInNode(std::shared_ptr<AVLNode> node, const std::string& pattern, std::vector<int>& results, int& currentPos) const;
    void searchRegexInNode(std::shared_ptr<AVLNode> node, const std::string& pattern, std::vector<int>& results, int& currentPos) const;
};

} // namespace TextFlow
