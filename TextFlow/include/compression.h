#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <queue>
#include <bitset>

namespace TextFlow {

// Huffman coding implementation
struct HuffmanNode {
    char character;
    int frequency;
    std::shared_ptr<HuffmanNode> left;
    std::shared_ptr<HuffmanNode> right;
    
    HuffmanNode(char c, int freq) : character(c), frequency(freq), left(nullptr), right(nullptr) {}
    HuffmanNode(int freq, std::shared_ptr<HuffmanNode> l, std::shared_ptr<HuffmanNode> r) 
        : character('\0'), frequency(freq), left(l), right(r) {}
    
    bool isLeaf() const { return !left && !right; }
};

class HuffmanCompression {
public:
    HuffmanCompression();
    ~HuffmanCompression() = default;
    
    // Compression
    std::vector<uint8_t> compress(const std::string& text);
    bool compressToFile(const std::string& text, const std::string& filename);
    
    // Decompression
    std::string decompress(const std::vector<uint8_t>& compressedData);
    std::string decompressFromFile(const std::string& filename);
    
    // Utility
    double getCompressionRatio(const std::string& original, const std::vector<uint8_t>& compressed) const;
    void printFrequencyTable() const;
    void printHuffmanCodes() const;

private:
    std::unordered_map<char, int> frequencyTable_;
    std::unordered_map<char, std::string> huffmanCodes_;
    std::shared_ptr<HuffmanNode> root_;
    
    void buildFrequencyTable(const std::string& text);
    void buildHuffmanTree();
    void generateHuffmanCodes();
    void generateCodesHelper(std::shared_ptr<HuffmanNode> node, const std::string& code);
    
    std::vector<uint8_t> encodeText(const std::string& text);
    std::string decodeText(const std::vector<uint8_t>& encodedData);
    
    // Bit manipulation helpers
    void writeBits(std::vector<uint8_t>& data, const std::string& bits);
    std::string readBits(const std::vector<uint8_t>& data, size_t& bitIndex, size_t bitCount);
    
    // Serialization
    void serializeTree(std::vector<uint8_t>& data);
    void deserializeTree(const std::vector<uint8_t>& data, size_t& index);
};

// LZ77 compression implementation
struct LZ77Token {
    int offset;
    int length;
    char nextChar;
    
    LZ77Token(int o, int l, char c) : offset(o), length(l), nextChar(c) {}
};

class LZ77Compression {
public:
    LZ77Compression(int windowSize = 32768, int bufferSize = 258);
    ~LZ77Compression() = default;
    
    // Compression
    std::vector<uint8_t> compress(const std::string& text);
    bool compressToFile(const std::string& text, const std::string& filename);
    
    // Decompression
    std::string decompress(const std::vector<uint8_t>& compressedData);
    std::string decompressFromFile(const std::string& filename);
    
    // Utility
    double getCompressionRatio(const std::string& original, const std::vector<uint8_t>& compressed) const;
    void printCompressionStats(const std::string& original, const std::vector<uint8_t>& compressed) const;

private:
    int windowSize_;
    int bufferSize_;
    
    std::vector<LZ77Token> encode(const std::string& text);
    std::string decode(const std::vector<LZ77Token>& tokens);
    
    std::pair<int, int> findLongestMatch(const std::string& text, int currentPos);
    std::vector<uint8_t> serializeTokens(const std::vector<LZ77Token>& tokens);
    std::vector<LZ77Token> deserializeTokens(const std::vector<uint8_t>& data);
};

// Compression manager
class CompressionManager {
public:
    enum class Algorithm {
        HUFFMAN,
        LZ77,
        AUTO
    };
    
    CompressionManager();
    ~CompressionManager() = default;
    
    // Main compression interface
    std::vector<uint8_t> compress(const std::string& text, Algorithm algorithm = Algorithm::AUTO);
    std::string decompress(const std::vector<uint8_t>& data);
    
    // File operations
    bool compressToFile(const std::string& text, const std::string& filename, Algorithm algorithm = Algorithm::AUTO);
    std::string decompressFromFile(const std::string& filename);
    
    // Analysis
    struct CompressionAnalysis {
        Algorithm bestAlgorithm;
        double huffmanRatio;
        double lz77Ratio;
        size_t originalSize;
        size_t compressedSize;
        double compressionRatio;
    };
    
    CompressionAnalysis analyzeCompression(const std::string& text);
    void printCompressionReport(const std::string& text);

private:
    std::unique_ptr<HuffmanCompression> huffman_;
    std::unique_ptr<LZ77Compression> lz77_;
    
    Algorithm selectBestAlgorithm(const std::string& text);
    std::vector<uint8_t> addAlgorithmHeader(const std::vector<uint8_t>& data, Algorithm algorithm);
    Algorithm readAlgorithmHeader(const std::vector<uint8_t>& data, size_t& index);
};

} // namespace TextFlow
