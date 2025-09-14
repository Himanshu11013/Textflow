#include <gtest/gtest.h>
#include "../include/avl_tree.h"
#include <string>
#include <vector>

using namespace TextFlow;

class AVLTreeTest : public ::testing::Test {
protected:
    void SetUp() override {
        tree = std::make_unique<AVLTree>();
    }
    
    std::unique_ptr<AVLTree> tree;
};

TEST_F(AVLTreeTest, EmptyTree) {
    EXPECT_EQ(tree->getSize(), 0);
    EXPECT_TRUE(tree->validate());
}

TEST_F(AVLTreeTest, SingleInsert) {
    tree->insert(0, "Hello");
    EXPECT_EQ(tree->getSize(), 5);
    EXPECT_EQ(tree->getText(0, 5), "Hello");
    EXPECT_TRUE(tree->validate());
}

TEST_F(AVLTreeTest, MultipleInserts) {
    tree->insert(0, "Hello");
    tree->insert(5, " World");
    tree->insert(11, "!");
    
    EXPECT_EQ(tree->getSize(), 12);
    EXPECT_EQ(tree->getText(0, 12), "Hello World!");
    EXPECT_TRUE(tree->validate());
}

TEST_F(AVLTreeTest, InsertInMiddle) {
    tree->insert(0, "Hello");
    tree->insert(5, " World");
    tree->insert(5, " Beautiful");
    
    EXPECT_EQ(tree->getSize(), 18);
    EXPECT_EQ(tree->getText(0, 18), "Hello Beautiful World");
    EXPECT_TRUE(tree->validate());
}

TEST_F(AVLTreeTest, DeleteText) {
    tree->insert(0, "Hello World");
    tree->erase(5, 6); // Delete " World"
    
    EXPECT_EQ(tree->getSize(), 5);
    EXPECT_EQ(tree->getText(0, 5), "Hello");
    EXPECT_TRUE(tree->validate());
}

TEST_F(AVLTreeTest, DeleteFromMiddle) {
    tree->insert(0, "Hello Beautiful World");
    tree->erase(6, 10); // Delete "Beautiful "
    
    EXPECT_EQ(tree->getSize(), 11);
    EXPECT_EQ(tree->getText(0, 11), "Hello World");
    EXPECT_TRUE(tree->validate());
}

TEST_F(AVLTreeTest, CursorMovement) {
    tree->insert(0, "Hello World");
    
    EXPECT_EQ(tree->moveCursor(0, 5), 5);
    EXPECT_EQ(tree->moveCursor(5, 3), 8);
    EXPECT_EQ(tree->moveCursor(8, -3), 5);
}

TEST_F(AVLTreeTest, LineOperations) {
    tree->insert(0, "Line 1\nLine 2\nLine 3");
    
    EXPECT_EQ(tree->getLineNumber(0), 1);
    EXPECT_EQ(tree->getLineNumber(7), 2);
    EXPECT_EQ(tree->getLineNumber(14), 3);
    
    EXPECT_EQ(tree->getLineStart(7), 7);
    EXPECT_EQ(tree->getLineEnd(7), 13);
}

TEST_F(AVLTreeTest, SearchOperations) {
    tree->insert(0, "Hello World Hello");
    
    auto results = tree->findAll("Hello");
    EXPECT_EQ(results.size(), 2);
    EXPECT_EQ(results[0], 0);
    EXPECT_EQ(results[1], 12);
}

TEST_F(AVLTreeTest, RegexSearch) {
    tree->insert(0, "Hello123World456");
    
    auto results = tree->findAllRegex("\\d+");
    EXPECT_EQ(results.size(), 2);
    EXPECT_EQ(results[0], 5);
    EXPECT_EQ(results[1], 11);
}

TEST_F(AVLTreeTest, UndoRedo) {
    tree->insert(0, "Hello");
    tree->saveState();
    
    tree->insert(5, " World");
    EXPECT_EQ(tree->getSize(), 11);
    
    tree->undo();
    EXPECT_EQ(tree->getSize(), 5);
    EXPECT_EQ(tree->getText(0, 5), "Hello");
    
    tree->redo();
    EXPECT_EQ(tree->getSize(), 11);
    EXPECT_EQ(tree->getText(0, 11), "Hello World");
}

TEST_F(AVLTreeTest, LargeText) {
    std::string largeText(10000, 'A');
    tree->insert(0, largeText);
    
    EXPECT_EQ(tree->getSize(), 10000);
    EXPECT_TRUE(tree->validate());
    
    // Test random access
    for (int i = 0; i < 100; ++i) {
        int pos = rand() % 10000;
        EXPECT_EQ(tree->getChar(pos), 'A');
    }
}

TEST_F(AVLTreeTest, StressTest) {
    // Insert random text at random positions
    std::vector<std::string> words = {"Hello", "World", "Test", "Data", "Structure"};
    
    for (int i = 0; i < 1000; ++i) {
        int pos = rand() % (tree->getSize() + 1);
        std::string word = words[rand() % words.size()];
        tree->insert(pos, word);
        
        if (i % 100 == 0) {
            EXPECT_TRUE(tree->validate());
        }
    }
    
    EXPECT_TRUE(tree->validate());
}

TEST_F(AVLTreeTest, FileOperations) {
    tree->insert(0, "Test file content\nWith multiple lines\nAnd various characters!");
    
    // Test save to file
    tree->saveToFile("test_output.txt");
    
    // Create new tree and load from file
    auto newTree = std::make_unique<AVLTree>();
    newTree->loadFromFile("test_output.txt");
    
    EXPECT_EQ(newTree->getSize(), tree->getSize());
    EXPECT_EQ(newTree->getText(0, newTree->getSize()), tree->getText(0, tree->getSize()));
    
    // Clean up
    std::remove("test_output.txt");
}

TEST_F(AVLTreeTest, CharacterAccess) {
    tree->insert(0, "Hello World");
    
    EXPECT_EQ(tree->getChar(0), 'H');
    EXPECT_EQ(tree->getChar(4), 'o');
    EXPECT_EQ(tree->getChar(5), ' ');
    EXPECT_EQ(tree->getChar(10), 'd');
    EXPECT_EQ(tree->getChar(11), '\0'); // Out of bounds
}

TEST_F(AVLTreeTest, TextRetrieval) {
    tree->insert(0, "Hello Beautiful World");
    
    EXPECT_EQ(tree->getText(0, 5), "Hello");
    EXPECT_EQ(tree->getText(6, 9), "Beautiful");
    EXPECT_EQ(tree->getText(16, 5), "World");
    EXPECT_EQ(tree->getText(0, 21), "Hello Beautiful World");
}

TEST_F(AVLTreeTest, EdgeCases) {
    // Empty insert
    tree->insert(0, "");
    EXPECT_EQ(tree->getSize(), 0);
    
    // Insert at end
    tree->insert(0, "Hello");
    tree->insert(5, " World");
    EXPECT_EQ(tree->getText(0, 11), "Hello World");
    
    // Delete beyond bounds
    tree->erase(10, 5);
    EXPECT_EQ(tree->getSize(), 11);
    
    // Delete all
    tree->erase(0, 11);
    EXPECT_EQ(tree->getSize(), 0);
}

TEST_F(AVLTreeTest, MemoryManagement) {
    // Test with many small inserts and deletes
    for (int i = 0; i < 1000; ++i) {
        tree->insert(0, "A");
        if (i % 2 == 0) {
            tree->erase(0, 1);
        }
    }
    
    EXPECT_TRUE(tree->validate());
}

TEST_F(AVLTreeTest, PerformanceTest) {
    // Measure insertion performance
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 10000; ++i) {
        tree->insert(i, "A");
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    EXPECT_LT(duration.count(), 1000); // Should complete in less than 1 second
    EXPECT_TRUE(tree->validate());
}
