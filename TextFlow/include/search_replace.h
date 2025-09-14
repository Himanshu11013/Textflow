#pragma once

#include <string>
#include <vector>
#include <regex>
#include <unordered_map>
#include <functional>

namespace TextFlow {

class SearchReplace {
public:
    SearchReplace();
    ~SearchReplace() = default;
    
    // KMP Algorithm for pattern matching
    std::vector<int> kmpSearch(const std::string& text, const std::string& pattern) const;
    
    // Boyer-Moore Algorithm for pattern matching
    std::vector<int> boyerMooreSearch(const std::string& text, const std::string& pattern) const;
    
    // Rabin-Karp Algorithm for pattern matching
    std::vector<int> rabinKarpSearch(const std::string& text, const std::string& pattern) const;
    
    // Regex search
    std::vector<int> regexSearch(const std::string& text, const std::string& pattern) const;
    
    // Case-insensitive search
    std::vector<int> caseInsensitiveSearch(const std::string& text, const std::string& pattern) const;
    
    // Whole word search
    std::vector<int> wholeWordSearch(const std::string& text, const std::string& pattern) const;
    
    // Multi-pattern search
    std::vector<std::pair<int, std::string>> multiPatternSearch(const std::string& text, 
                                                               const std::vector<std::string>& patterns) const;
    
    // Replace operations
    std::string replaceAll(const std::string& text, const std::string& pattern, 
                          const std::string& replacement) const;
    std::string replaceFirst(const std::string& text, const std::string& pattern, 
                            const std::string& replacement) const;
    std::string replaceRegex(const std::string& text, const std::string& pattern, 
                            const std::string& replacement) const;
    
    // Advanced replace with callback
    std::string replaceWithCallback(const std::string& text, const std::string& pattern,
                                   std::function<std::string(const std::string&)> callback) const;
    
    // Search and highlight
    struct SearchResult {
        int position;
        int length;
        std::string matchedText;
        std::string context; // Surrounding text
    };
    
    std::vector<SearchResult> searchWithContext(const std::string& text, const std::string& pattern,
                                               int contextLength = 50) const;
    
    // Batch operations
    struct BatchReplace {
        std::string pattern;
        std::string replacement;
        bool useRegex;
        bool caseSensitive;
    };
    
    std::string batchReplace(const std::string& text, const std::vector<BatchReplace>& operations) const;
    
    // Search statistics
    struct SearchStats {
        int totalMatches;
        int uniqueMatches;
        std::unordered_map<std::string, int> patternCounts;
        double averageMatchLength;
    };
    
    SearchStats getSearchStats(const std::string& text, const std::vector<std::string>& patterns) const;

private:
    // KMP helper methods
    std::vector<int> computeLPSArray(const std::string& pattern) const;
    
    // Boyer-Moore helper methods
    std::unordered_map<char, int> buildBadCharTable(const std::string& pattern) const;
    std::vector<int> buildGoodSuffixTable(const std::string& pattern) const;
    
    // Rabin-Karp helper methods
    long long computeHash(const std::string& str, int start, int length, long long base, long long mod) const;
    long long computePower(long long base, int exp, long long mod) const;
    
    // Utility methods
    std::string toLowerCase(const std::string& str) const;
    bool isWordBoundary(const std::string& text, int position) const;
    std::string extractContext(const std::string& text, int position, int length, int contextLength) const;
};

} // namespace TextFlow
