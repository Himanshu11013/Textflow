#include "search_replace.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <sstream>
#include <unordered_set>
#include <stdexcept>
#include <regex>
#include <utility>

namespace TextFlow {

SearchReplace::SearchReplace() = default;

std::vector<int> SearchReplace::kmpSearch(const std::string& text, const std::string& pattern) const {
    std::vector<int> result;
    if (pattern.empty() || text.length() < pattern.length()) {
        return result;
    }
    
    std::vector<int> lps = computeLPSArray(pattern);
    int textLen = text.length();
    int patternLen = pattern.length();
    
    int i = 0; // index for text
    int j = 0; // index for pattern
    
    while (i < textLen) {
        if (pattern[j] == text[i]) {
            i++;
            j++;
        }
        
        if (j == patternLen) {
            result.push_back(i - j);
            j = lps[j - 1];
        } else if (i < textLen && pattern[j] != text[i]) {
            if (j != 0) {
                j = lps[j - 1];
            } else {
                i++;
            }
        }
    }
    
    return result;
}

std::vector<int> SearchReplace::boyerMooreSearch(const std::string& text, const std::string& pattern) const {
    std::vector<int> result;
    if (pattern.empty() || text.length() < pattern.length()) {
        return result;
    }
    
    auto badCharTable = buildBadCharTable(pattern);
    auto goodSuffixTable = buildGoodSuffixTable(pattern);
    
    int textLen = text.length();
    int patternLen = pattern.length();
    int shift = 0;
    
    while (shift <= textLen - patternLen) {
        int j = patternLen - 1;
        
        // Keep reducing index j of pattern while characters of pattern and text are matching
        while (j >= 0 && pattern[j] == text[shift + j]) {
            j--;
        }
        
        if (j < 0) {
            result.push_back(shift);
            // Shift the pattern so that the next character in text aligns with the last occurrence of it in pattern
            shift += (shift + patternLen < textLen) ? 
                     patternLen - badCharTable[text[shift + patternLen]] : 1;
        } else {
            // Shift the pattern so that the bad character in text aligns with the last occurrence of it in pattern
            shift += std::max(goodSuffixTable[j], j - badCharTable[text[shift + j]]);
        }
    }
    
    return result;
}

std::vector<int> SearchReplace::rabinKarpSearch(const std::string& text, const std::string& pattern) const {
    std::vector<int> result;
    if (pattern.empty() || text.length() < pattern.length()) {
        return result;
    }
    
    const long long base = 256;
    const long long mod = 1000000007;
    
    int textLen = text.length();
    int patternLen = pattern.length();
    
    long long patternHash = computeHash(pattern, 0, patternLen, base, mod);
    long long textHash = computeHash(text, 0, patternLen, base, mod);
    long long power = computePower(base, patternLen - 1, mod);
    
    for (int i = 0; i <= textLen - patternLen; i++) {
        if (patternHash == textHash) {
            // Verify the match by comparing characters
            bool match = true;
            for (int j = 0; j < patternLen; j++) {
                if (text[i + j] != pattern[j]) {
                    match = false;
                    break;
                }
            }
            if (match) {
                result.push_back(i);
            }
        }
        
        // Calculate hash for next window
        if (i < textLen - patternLen) {
            textHash = (base * (textHash - text[i] * power) + text[i + patternLen]) % mod;
            if (textHash < 0) {
                textHash += mod;
            }
        }
    }
    
    return result;
}

std::vector<int> SearchReplace::regexSearch(const std::string& text, const std::string& pattern) const {
    std::vector<int> result;
    
    try {
        std::regex regexPattern(pattern);
        std::sregex_iterator iter(text.begin(), text.end(), regexPattern);
        std::sregex_iterator end;
        
        for (; iter != end; ++iter) {
            result.push_back(static_cast<int>(iter->position()));
        }
    } catch (const std::regex_error& e) {
        // Invalid regex pattern, return empty result
    }
    
    return result;
}

std::vector<int> SearchReplace::caseInsensitiveSearch(const std::string& text, const std::string& pattern) const {
    std::string lowerText = toLowerCase(text);
    std::string lowerPattern = toLowerCase(pattern);
    
    return kmpSearch(lowerText, lowerPattern);
}

std::vector<int> SearchReplace::wholeWordSearch(const std::string& text, const std::string& pattern) const {
    std::vector<int> result = kmpSearch(text, pattern);
    
    // Filter results to only include whole word matches
    result.erase(std::remove_if(result.begin(), result.end(),
        [&](int pos) {
            return !isWordBoundary(text, pos) || 
                   !isWordBoundary(text, pos + static_cast<int>(pattern.length()));
        }), result.end());
    
    return result;
}

std::vector<std::pair<int, std::string>> SearchReplace::multiPatternSearch(const std::string& text, 
                                                                          const std::vector<std::string>& patterns) const {
    std::vector<std::pair<int, std::string>> result;
    
    for (const auto& pattern : patterns) {
        std::vector<int> positions = kmpSearch(text, pattern);
        for (int pos : positions) {
            result.emplace_back(pos, pattern);
        }
    }
    
    // Sort by position
    std::sort(result.begin(), result.end());
    
    return result;
}

std::string SearchReplace::replaceAll(const std::string& text, const std::string& pattern, 
                                     const std::string& replacement) const {
    if (pattern.empty()) return text;
    
    std::string result = text;
    std::vector<int> positions = kmpSearch(text, pattern);
    
    // Replace from end to beginning to maintain correct positions
    for (auto it = positions.rbegin(); it != positions.rend(); ++it) {
        result.replace(*it, pattern.length(), replacement);
    }
    
    return result;
}

std::string SearchReplace::replaceFirst(const std::string& text, const std::string& pattern, 
                                       const std::string& replacement) const {
    if (pattern.empty()) return text;
    
    std::vector<int> positions = kmpSearch(text, pattern);
    if (positions.empty()) return text;
    
    std::string result = text;
    int pos = positions[0];
    result.replace(pos, pattern.length(), replacement);
    
    return result;
}

std::string SearchReplace::replaceRegex(const std::string& text, const std::string& pattern, 
                                       const std::string& replacement) const {
    try {
        std::regex regexPattern(pattern);
        return std::regex_replace(text, regexPattern, replacement);
    } catch (const std::regex_error& e) {
        return text; // Return original text if regex is invalid
    }
}

std::string SearchReplace::replaceWithCallback(const std::string& text, const std::string& pattern,
                                              std::function<std::string(const std::string&)> callback) const {
    std::string result = text;
    std::vector<int> positions = kmpSearch(text, pattern);
    
    // Replace from end to beginning
    for (auto it = positions.rbegin(); it != positions.rend(); ++it) {
        std::string matchedText = text.substr(*it, pattern.length());
        std::string replacement = callback(matchedText);
        result.replace(*it, pattern.length(), replacement);
    }
    
    return result;
}

std::vector<SearchReplace::SearchResult> SearchReplace::searchWithContext(const std::string& text, 
                                                                         const std::string& pattern,
                                                                         int contextLength) const {
    std::vector<SearchResult> result;
    std::vector<int> positions = kmpSearch(text, pattern);
    
    for (int pos : positions) {
        SearchResult searchResult;
        searchResult.position = pos;
        searchResult.length = static_cast<int>(pattern.length());
        searchResult.matchedText = text.substr(pos, pattern.length());
        searchResult.context = extractContext(text, pos, static_cast<int>(pattern.length()), contextLength);
        result.push_back(searchResult);
    }
    
    return result;
}

std::string SearchReplace::batchReplace(const std::string& text, const std::vector<BatchReplace>& operations) const {
    std::string result = text;
    
    for (const auto& op : operations) {
        if (op.useRegex) {
            result = replaceRegex(result, op.pattern, op.replacement);
        } else if (op.caseSensitive) {
            result = replaceAll(result, op.pattern, op.replacement);
        } else {
            // Case-insensitive replacement
            std::string lowerText = toLowerCase(result);
            std::string lowerPattern = toLowerCase(op.pattern);
            std::vector<int> positions = kmpSearch(lowerText, lowerPattern);
            
            for (auto it = positions.rbegin(); it != positions.rend(); ++it) {
                result.replace(*it, op.pattern.length(), op.replacement);
            }
        }
    }
    
    return result;
}

SearchReplace::SearchStats SearchReplace::getSearchStats(const std::string& text, 
                                                        const std::vector<std::string>& patterns) const {
    SearchStats stats;
    stats.totalMatches = 0;
    stats.uniqueMatches = 0;
    stats.averageMatchLength = 0.0;
    
    std::unordered_set<std::string> uniqueMatches;
    int totalLength = 0;
    
    for (const auto& pattern : patterns) {
        std::vector<int> positions = kmpSearch(text, pattern);
        stats.totalMatches += positions.size();
        stats.patternCounts[pattern] = positions.size();
        
        for (int pos : positions) {
            std::string match = text.substr(pos, pattern.length());
            uniqueMatches.insert(match);
            totalLength += static_cast<int>(match.length());
        }
    }
    
    stats.uniqueMatches = static_cast<int>(uniqueMatches.size());
    if (stats.totalMatches > 0) {
        stats.averageMatchLength = static_cast<double>(totalLength) / stats.totalMatches;
    }
    
    return stats;
}

// Private helper methods

std::vector<int> SearchReplace::computeLPSArray(const std::string& pattern) const {
    int patternLen = pattern.length();
    std::vector<int> lps(patternLen, 0);
    
    int len = 0;
    int i = 1;
    
    while (i < patternLen) {
        if (pattern[i] == pattern[len]) {
            len++;
            lps[i] = len;
            i++;
        } else {
            if (len != 0) {
                len = lps[len - 1];
            } else {
                lps[i] = 0;
                i++;
            }
        }
    }
    
    return lps;
}

std::unordered_map<char, int> SearchReplace::buildBadCharTable(const std::string& pattern) const {
    std::unordered_map<char, int> table;
    int patternLen = pattern.length();
    
    for (int i = 0; i < patternLen - 1; i++) {
        table[pattern[i]] = patternLen - 1 - i;
    }
    
    return table;
}

std::vector<int> SearchReplace::buildGoodSuffixTable(const std::string& pattern) const {
    int patternLen = pattern.length();
    std::vector<int> table(patternLen, 0);
    
    // Simplified implementation - in practice, this would be more complex
    for (int i = 0; i < patternLen; i++) {
        table[i] = patternLen;
    }
    
    return table;
}

long long SearchReplace::computeHash(const std::string& str, int start, int length, long long base, long long mod) const {
    long long hash = 0;
    long long power = 1;
    
    for (int i = start + length - 1; i >= start; i--) {
        hash = (hash + (str[i] * power) % mod) % mod;
        power = (power * base) % mod;
    }
    
    return hash;
}

long long SearchReplace::computePower(long long base, int exp, long long mod) const {
    long long result = 1;
    base = base % mod;
    
    while (exp > 0) {
        if (exp % 2 == 1) {
            result = (result * base) % mod;
        }
        exp = exp >> 1;
        base = (base * base) % mod;
    }
    
    return result;
}

std::string SearchReplace::toLowerCase(const std::string& str) const {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

bool SearchReplace::isWordBoundary(const std::string& text, int position) const {
    if (position == 0 || position == static_cast<int>(text.length())) {
        return true;
    }
    
    char prev = text[position - 1];
    char curr = text[position];
    
    return (!std::isalnum(prev) && prev != '_') || (!std::isalnum(curr) && curr != '_');
}

std::string SearchReplace::extractContext(const std::string& text, int position, int length, int contextLength) const {
    int start = std::max(0, position - contextLength);
    int end = std::min(static_cast<int>(text.length()), position + length + contextLength);
    
    std::string context = text.substr(start, end - start);
    
    // Add ellipsis if we're not at the beginning/end
    if (start > 0) {
        context = "..." + context;
    }
    if (end < static_cast<int>(text.length())) {
        context = context + "...";
    }
    
    return context;
}

} // namespace TextFlow
