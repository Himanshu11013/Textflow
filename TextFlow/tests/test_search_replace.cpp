#include <gtest/gtest.h>
#include "../include/search_replace.h"
#include <string>
#include <vector>

using namespace TextFlow;

class SearchReplaceTest : public ::testing::Test {
protected:
    void SetUp() override {
        searchReplace = std::make_unique<SearchReplace>();
    }
    
    std::unique_ptr<SearchReplace> searchReplace;
};

TEST_F(SearchReplaceTest, KMPSearch) {
    std::string text = "Hello World Hello";
    std::string pattern = "Hello";
    
    auto results = searchReplace->kmpSearch(text, pattern);
    
    EXPECT_EQ(results.size(), 2);
    EXPECT_EQ(results[0], 0);
    EXPECT_EQ(results[1], 12);
}

TEST_F(SearchReplaceTest, KMPSearchNotFound) {
    std::string text = "Hello World";
    std::string pattern = "Goodbye";
    
    auto results = searchReplace->kmpSearch(text, pattern);
    
    EXPECT_TRUE(results.empty());
}

TEST_F(SearchReplaceTest, BoyerMooreSearch) {
    std::string text = "Hello World Hello";
    std::string pattern = "World";
    
    auto results = searchReplace->boyerMooreSearch(text, pattern);
    
    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0], 6);
}

TEST_F(SearchReplaceTest, RabinKarpSearch) {
    std::string text = "Hello World Hello";
    std::string pattern = "Hello";
    
    auto results = searchReplace->rabinKarpSearch(text, pattern);
    
    EXPECT_EQ(results.size(), 2);
    EXPECT_EQ(results[0], 0);
    EXPECT_EQ(results[1], 12);
}

TEST_F(SearchReplaceTest, RegexSearch) {
    std::string text = "Hello123World456";
    std::string pattern = "\\d+";
    
    auto results = searchReplace->regexSearch(text, pattern);
    
    EXPECT_EQ(results.size(), 2);
    EXPECT_EQ(results[0], 5);
    EXPECT_EQ(results[1], 11);
}

TEST_F(SearchReplaceTest, CaseInsensitiveSearch) {
    std::string text = "Hello World hello";
    std::string pattern = "hello";
    
    auto results = searchReplace->caseInsensitiveSearch(text, pattern);
    
    EXPECT_EQ(results.size(), 2);
    EXPECT_EQ(results[0], 0);
    EXPECT_EQ(results[1], 12);
}

TEST_F(SearchReplaceTest, WholeWordSearch) {
    std::string text = "Hello World HelloWorld";
    std::string pattern = "Hello";
    
    auto results = searchReplace->wholeWordSearch(text, pattern);
    
    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0], 0);
}

TEST_F(SearchReplaceTest, MultiPatternSearch) {
    std::string text = "Hello World Test";
    std::vector<std::string> patterns = {"Hello", "World", "Test"};
    
    auto results = searchReplace->multiPatternSearch(text, patterns);
    
    EXPECT_EQ(results.size(), 3);
    EXPECT_EQ(results[0].first, 0);
    EXPECT_EQ(results[0].second, "Hello");
    EXPECT_EQ(results[1].first, 6);
    EXPECT_EQ(results[1].second, "World");
    EXPECT_EQ(results[2].first, 12);
    EXPECT_EQ(results[2].second, "Test");
}

TEST_F(SearchReplaceTest, ReplaceAll) {
    std::string text = "Hello World Hello";
    std::string pattern = "Hello";
    std::string replacement = "Hi";
    
    std::string result = searchReplace->replaceAll(text, pattern, replacement);
    
    EXPECT_EQ(result, "Hi World Hi");
}

TEST_F(SearchReplaceTest, ReplaceFirst) {
    std::string text = "Hello World Hello";
    std::string pattern = "Hello";
    std::string replacement = "Hi";
    
    std::string result = searchReplace->replaceFirst(text, pattern, replacement);
    
    EXPECT_EQ(result, "Hi World Hello");
}

TEST_F(SearchReplaceTest, ReplaceRegex) {
    std::string text = "Hello123World456";
    std::string pattern = "\\d+";
    std::string replacement = "X";
    
    std::string result = searchReplace->replaceRegex(text, pattern, replacement);
    
    EXPECT_EQ(result, "HelloXWorldX");
}

TEST_F(SearchReplaceTest, ReplaceWithCallback) {
    std::string text = "Hello World Hello";
    std::string pattern = "Hello";
    
    auto callback = [](const std::string& match) -> std::string {
        return match + "!";
    };
    
    std::string result = searchReplace->replaceWithCallback(text, pattern, callback);
    
    EXPECT_EQ(result, "Hello! World Hello!");
}

TEST_F(SearchReplaceTest, SearchWithContext) {
    std::string text = "This is a test sentence with Hello World in it.";
    std::string pattern = "Hello World";
    
    auto results = searchReplace->searchWithContext(text, pattern, 10);
    
    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].position, 30);
    EXPECT_EQ(results[0].length, 11);
    EXPECT_EQ(results[0].matchedText, "Hello World");
    EXPECT_TRUE(results[0].context.find("Hello World") != std::string::npos);
}

TEST_F(SearchReplaceTest, BatchReplace) {
    std::string text = "Hello World Test";
    
    std::vector<SearchReplace::BatchReplace> operations = {
        {"Hello", "Hi", false, true},
        {"World", "Universe", false, true},
        {"Test", "Example", false, true}
    };
    
    std::string result = searchReplace->batchReplace(text, operations);
    
    EXPECT_EQ(result, "Hi Universe Example");
}

TEST_F(SearchReplaceTest, SearchStats) {
    std::string text = "Hello World Hello Test World";
    std::vector<std::string> patterns = {"Hello", "World", "Test"};
    
    auto stats = searchReplace->getSearchStats(text, patterns);
    
    EXPECT_EQ(stats.totalMatches, 5);
    EXPECT_EQ(stats.uniqueMatches, 3);
    EXPECT_EQ(stats.patternCounts["Hello"], 2);
    EXPECT_EQ(stats.patternCounts["World"], 2);
    EXPECT_EQ(stats.patternCounts["Test"], 1);
    EXPECT_GT(stats.averageMatchLength, 0);
}

TEST_F(SearchReplaceTest, EmptyPattern) {
    std::string text = "Hello World";
    std::string pattern = "";
    
    auto results = searchReplace->kmpSearch(text, pattern);
    
    EXPECT_TRUE(results.empty());
}

TEST_F(SearchReplaceTest, EmptyText) {
    std::string text = "";
    std::string pattern = "Hello";
    
    auto results = searchReplace->kmpSearch(text, pattern);
    
    EXPECT_TRUE(results.empty());
}

TEST_F(SearchReplaceTest, PatternLongerThanText) {
    std::string text = "Hi";
    std::string pattern = "Hello";
    
    auto results = searchReplace->kmpSearch(text, pattern);
    
    EXPECT_TRUE(results.empty());
}

TEST_F(SearchReplaceTest, SpecialCharacters) {
    std::string text = "Hello! World? Test.";
    std::string pattern = "!";
    
    auto results = searchReplace->kmpSearch(text, pattern);
    
    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0], 5);
}

TEST_F(SearchReplaceTest, UnicodeText) {
    std::string text = "Hello 世界 World";
    std::string pattern = "世界";
    
    auto results = searchReplace->kmpSearch(text, pattern);
    
    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0], 6);
}

TEST_F(SearchReplaceTest, PerformanceTest) {
    std::string text(10000, 'A');
    text += "Hello";
    text += std::string(10000, 'A');
    
    std::string pattern = "Hello";
    
    auto start = std::chrono::high_resolution_clock::now();
    auto results = searchReplace->kmpSearch(text, pattern);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    EXPECT_LT(duration.count(), 1000); // Should complete in less than 1ms
    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0], 10000);
}

TEST_F(SearchReplaceTest, LargePattern) {
    std::string text = "Hello World Hello World";
    std::string pattern = "Hello World Hello World";
    
    auto results = searchReplace->kmpSearch(text, pattern);
    
    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0], 0);
}

TEST_F(SearchReplaceTest, OverlappingPatterns) {
    std::string text = "AAAAA";
    std::string pattern = "AA";
    
    auto results = searchReplace->kmpSearch(text, pattern);
    
    EXPECT_EQ(results.size(), 4);
    EXPECT_EQ(results[0], 0);
    EXPECT_EQ(results[1], 1);
    EXPECT_EQ(results[2], 2);
    EXPECT_EQ(results[3], 3);
}
