#include "compression.h"
#include <fstream>
#include <algorithm>
#include <iostream>
#include <iomanip>

namespace TextFlow {

// HuffmanCompression implementation

HuffmanCompression::HuffmanCompression() : root_(nullptr) {}

std::vector<uint8_t> HuffmanCompression::compress(const std::string& text) {
    if (text.empty()) return {};
    
    buildFrequencyTable(text);
    buildHuffmanTree();
    generateHuffmanCodes();
    
    std::vector<uint8_t> result;
    
    // Serialize the Huffman tree
    serializeTree(result);
    
    // Encode the text
    std::vector<uint8_t> encodedText = encodeText(text);
    result.insert(result.end(), encodedText.begin(), encodedText.end());
    
    return result;
}

bool HuffmanCompression::compressToFile(const std::string& text, const std::string& filename) {
    try {
        std::vector<uint8_t> compressed = compress(text);
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) return false;
        
        file.write(reinterpret_cast<const char*>(compressed.data()), compressed.size());
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Compression error: " << e.what() << std::endl;
        return false;
    }
}

std::string HuffmanCompression::decompress(const std::vector<uint8_t>& compressedData) {
    if (compressedData.empty()) return "";
    
    size_t index = 0;
    deserializeTree(compressedData, index);
    
    return decodeText(compressedData);
}

std::string HuffmanCompression::decompressFromFile(const std::string& filename) {
    try {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) return "";
        
        file.seekg(0, std::ios::end);
        size_t size = file.tellg();
        file.seekg(0, std::ios::beg);
        
        std::vector<uint8_t> data(size);
        file.read(reinterpret_cast<char*>(data.data()), size);
        
        return decompress(data);
    } catch (const std::exception& e) {
        std::cerr << "Decompression error: " << e.what() << std::endl;
        return "";
    }
}

double HuffmanCompression::getCompressionRatio(const std::string& original, const std::vector<uint8_t>& compressed) const {
    if (original.empty()) return 0.0;
    return static_cast<double>(compressed.size()) / original.length();
}

void HuffmanCompression::printFrequencyTable() const {
    std::cout << "Character Frequency Table:\n";
    for (const auto& pair : frequencyTable_) {
        std::cout << "'" << pair.first << "': " << pair.second << std::endl;
    }
}

void HuffmanCompression::printHuffmanCodes() const {
    std::cout << "Huffman Codes:\n";
    for (const auto& pair : huffmanCodes_) {
        std::cout << "'" << pair.first << "': " << pair.second << std::endl;
    }
}

void HuffmanCompression::buildFrequencyTable(const std::string& text) {
    frequencyTable_.clear();
    for (char c : text) {
        frequencyTable_[c]++;
    }
}

void HuffmanCompression::buildHuffmanTree() {
    auto compare = [](const std::shared_ptr<HuffmanNode>& a, const std::shared_ptr<HuffmanNode>& b) {
        return a->frequency > b->frequency;
    };
    
    std::priority_queue<std::shared_ptr<HuffmanNode>, std::vector<std::shared_ptr<HuffmanNode>>, decltype(compare)> pq(compare);
    
    // Create leaf nodes for each character
    for (const auto& pair : frequencyTable_) {
        pq.push(std::make_shared<HuffmanNode>(pair.first, pair.second));
    }
    
    // Build the tree
    while (pq.size() > 1) {
        auto left = pq.top(); pq.pop();
        auto right = pq.top(); pq.pop();
        
        auto merged = std::make_shared<HuffmanNode>(left->frequency + right->frequency, left, right);
        pq.push(merged);
    }
    
    root_ = pq.empty() ? nullptr : pq.top();
}

void HuffmanCompression::generateHuffmanCodes() {
    huffmanCodes_.clear();
    if (root_) {
        if (root_->isLeaf()) {
            // Special case: only one character
            huffmanCodes_[root_->character] = "0";
        } else {
            generateCodesHelper(root_, "");
        }
    }
}

void HuffmanCompression::generateCodesHelper(std::shared_ptr<HuffmanNode> node, const std::string& code) {
    if (!node) return;
    
    if (node->isLeaf()) {
        huffmanCodes_[node->character] = code;
    } else {
        generateCodesHelper(node->left, code + "0");
        generateCodesHelper(node->right, code + "1");
    }
}

std::vector<uint8_t> HuffmanCompression::encodeText(const std::string& text) {
    std::string bitString;
    
    // Convert text to bit string using Huffman codes
    for (char c : text) {
        auto it = huffmanCodes_.find(c);
        if (it != huffmanCodes_.end()) {
            bitString += it->second;
        }
    }
    
    // Convert bit string to bytes
    std::vector<uint8_t> result;
    writeBits(result, bitString);
    
    return result;
}

std::string HuffmanCompression::decodeText(const std::vector<uint8_t>& encodedData) {
    if (!root_) return "";
    
    std::string result;
    size_t bitIndex = 0;
    
    // Skip the tree data (we need to find where the encoded text starts)
    // This is a simplified implementation - in practice, we'd store the tree size
    size_t treeStart = 0;
    while (treeStart < encodedData.size() && encodedData[treeStart] != 0xFF) {
        treeStart++;
    }
    treeStart++; // Skip the separator
    
    // Decode the text
    std::shared_ptr<HuffmanNode> current = root_;
    
    for (size_t i = treeStart; i < encodedData.size(); i++) {
        for (int bit = 7; bit >= 0; bit--) {
            bool bitValue = (encodedData[i] >> bit) & 1;
            
            if (bitValue) {
                current = current->right;
            } else {
                current = current->left;
            }
            
            if (current && current->isLeaf()) {
                result += current->character;
                current = root_;
            }
        }
    }
    
    return result;
}

void HuffmanCompression::writeBits(std::vector<uint8_t>& data, const std::string& bits) {
    uint8_t currentByte = 0;
    int bitCount = 0;
    
    for (char bit : bits) {
        if (bit == '1') {
            currentByte |= (1 << (7 - bitCount));
        }
        bitCount++;
        
        if (bitCount == 8) {
            data.push_back(currentByte);
            currentByte = 0;
            bitCount = 0;
        }
    }
    
    if (bitCount > 0) {
        data.push_back(currentByte);
    }
}

std::string HuffmanCompression::readBits(const std::vector<uint8_t>& data, size_t& bitIndex, size_t bitCount) {
    std::string result;
    
    for (size_t i = 0; i < bitCount; i++) {
        size_t byteIndex = bitIndex / 8;
        size_t bitInByte = bitIndex % 8;
        
        if (byteIndex >= data.size()) break;
        
        bool bit = (data[byteIndex] >> (7 - bitInByte)) & 1;
        result += bit ? '1' : '0';
        bitIndex++;
    }
    
    return result;
}

void HuffmanCompression::serializeTree(std::vector<uint8_t>& data) {
    // Simplified tree serialization
    // In practice, this would be more sophisticated
    data.push_back(0xFF); // Tree separator
}

void HuffmanCompression::deserializeTree(const std::vector<uint8_t>& data, size_t& index) {
    // Simplified tree deserialization
    // Find the separator
    while (index < data.size() && data[index] != 0xFF) {
        index++;
    }
    index++; // Skip separator
}

// LZ77Compression implementation

LZ77Compression::LZ77Compression(int windowSize, int bufferSize) 
    : windowSize_(windowSize), bufferSize_(bufferSize) {}

std::vector<uint8_t> LZ77Compression::compress(const std::string& text) {
    if (text.empty()) return {};
    
    std::vector<LZ77Token> tokens = encode(text);
    return serializeTokens(tokens);
}

bool LZ77Compression::compressToFile(const std::string& text, const std::string& filename) {
    try {
        std::vector<uint8_t> compressed = compress(text);
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) return false;
        
        file.write(reinterpret_cast<const char*>(compressed.data()), compressed.size());
        return true;
    } catch (const std::exception& e) {
        std::cerr << "LZ77 compression error: " << e.what() << std::endl;
        return false;
    }
}

std::string LZ77Compression::decompress(const std::vector<uint8_t>& compressedData) {
    if (compressedData.empty()) return "";
    
    std::vector<LZ77Token> tokens = deserializeTokens(compressedData);
    return decode(tokens);
}

std::string LZ77Compression::decompressFromFile(const std::string& filename) {
    try {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) return "";
        
        file.seekg(0, std::ios::end);
        size_t size = file.tellg();
        file.seekg(0, std::ios::beg);
        
        std::vector<uint8_t> data(size);
        file.read(reinterpret_cast<char*>(data.data()), size);
        
        return decompress(data);
    } catch (const std::exception& e) {
        std::cerr << "LZ77 decompression error: " << e.what() << std::endl;
        return "";
    }
}

double LZ77Compression::getCompressionRatio(const std::string& original, const std::vector<uint8_t>& compressed) const {
    if (original.empty()) return 0.0;
    return static_cast<double>(compressed.size()) / original.length();
}

void LZ77Compression::printCompressionStats(const std::string& original, const std::vector<uint8_t>& compressed) const {
    std::cout << "LZ77 Compression Stats:\n";
    std::cout << "Original size: " << original.length() << " bytes\n";
    std::cout << "Compressed size: " << compressed.size() << " bytes\n";
    std::cout << "Compression ratio: " << std::fixed << std::setprecision(2) 
              << getCompressionRatio(original, compressed) << "\n";
}

std::vector<LZ77Token> LZ77Compression::encode(const std::string& text) {
    std::vector<LZ77Token> tokens;
    int pos = 0;
    
    while (pos < static_cast<int>(text.length())) {
        auto match = findLongestMatch(text, pos);
        
        if (match.first > 0 && match.second > 0) {
            // Found a match
            tokens.emplace_back(match.first, match.second, text[pos + match.second]);
            pos += match.second + 1;
        } else {
            // No match found
            tokens.emplace_back(0, 0, text[pos]);
            pos++;
        }
    }
    
    return tokens;
}

std::string LZ77Compression::decode(const std::vector<LZ77Token>& tokens) {
    std::string result;
    
    for (const auto& token : tokens) {
        if (token.offset == 0) {
            // Literal character
            result += token.nextChar;
        } else {
            // Copy from previous position
            int start = result.length() - token.offset;
            for (int i = 0; i < token.length; i++) {
                result += result[start + i];
            }
            result += token.nextChar;
        }
    }
    
    return result;
}

std::pair<int, int> LZ77Compression::findLongestMatch(const std::string& text, int currentPos) {
    int bestOffset = 0;
    int bestLength = 0;
    
    int searchStart = std::max(0, currentPos - windowSize_);
    int searchEnd = currentPos;
    int maxLength = std::min(bufferSize_, static_cast<int>(text.length()) - currentPos);
    
    for (int i = searchStart; i < searchEnd; i++) {
        int length = 0;
        while (length < maxLength && 
               i + length < currentPos && 
               currentPos + length < static_cast<int>(text.length()) &&
               text[i + length] == text[currentPos + length]) {
            length++;
        }
        
        if (length > bestLength) {
            bestLength = length;
            bestOffset = currentPos - i;
        }
    }
    
    return {bestOffset, bestLength};
}

std::vector<uint8_t> LZ77Compression::serializeTokens(const std::vector<LZ77Token>& tokens) {
    std::vector<uint8_t> result;
    
    for (const auto& token : tokens) {
        // Pack offset (16 bits), length (8 bits), and character (8 bits) into 4 bytes
        result.push_back((token.offset >> 8) & 0xFF);
        result.push_back(token.offset & 0xFF);
        result.push_back(token.length & 0xFF);
        result.push_back(static_cast<uint8_t>(token.nextChar));
    }
    
    return result;
}

std::vector<LZ77Token> LZ77Compression::deserializeTokens(const std::vector<uint8_t>& data) {
    std::vector<LZ77Token> tokens;
    
    for (size_t i = 0; i < data.size(); i += 4) {
        if (i + 3 < data.size()) {
            int offset = (data[i] << 8) | data[i + 1];
            int length = data[i + 2];
            char nextChar = data[i + 3];
            tokens.emplace_back(offset, length, nextChar);
        }
    }
    
    return tokens;
}

// CompressionManager implementation

CompressionManager::CompressionManager() 
    : huffman_(std::make_unique<HuffmanCompression>())
    , lz77_(std::make_unique<LZ77Compression>()) {}

std::vector<uint8_t> CompressionManager::compress(const std::string& text, Algorithm algorithm) {
    if (algorithm == Algorithm::AUTO) {
        algorithm = selectBestAlgorithm(text);
    }
    
    std::vector<uint8_t> compressed;
    
    switch (algorithm) {
        case Algorithm::HUFFMAN:
            compressed = huffman_->compress(text);
            break;
        case Algorithm::LZ77:
            compressed = lz77_->compress(text);
            break;
    }
    
    return addAlgorithmHeader(compressed, algorithm);
}

std::string CompressionManager::decompress(const std::vector<uint8_t>& data) {
    if (data.empty()) return "";
    
    size_t index = 0;
    Algorithm algorithm = readAlgorithmHeader(data, index);
    
    std::vector<uint8_t> compressedData(data.begin() + index, data.end());
    
    switch (algorithm) {
        case Algorithm::HUFFMAN:
            return huffman_->decompress(compressedData);
        case Algorithm::LZ77:
            return lz77_->decompress(compressedData);
        default:
            return "";
    }
}

bool CompressionManager::compressToFile(const std::string& text, const std::string& filename, Algorithm algorithm) {
    try {
        std::vector<uint8_t> compressed = compress(text, algorithm);
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) return false;
        
        file.write(reinterpret_cast<const char*>(compressed.data()), compressed.size());
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Compression error: " << e.what() << std::endl;
        return false;
    }
}

std::string CompressionManager::decompressFromFile(const std::string& filename) {
    try {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) return "";
        
        file.seekg(0, std::ios::end);
        size_t size = file.tellg();
        file.seekg(0, std::ios::beg);
        
        std::vector<uint8_t> data(size);
        file.read(reinterpret_cast<char*>(data.data()), size);
        
        return decompress(data);
    } catch (const std::exception& e) {
        std::cerr << "Decompression error: " << e.what() << std::endl;
        return "";
    }
}

CompressionManager::CompressionAnalysis CompressionManager::analyzeCompression(const std::string& text) {
    CompressionAnalysis analysis;
    analysis.originalSize = text.length();
    
    auto huffmanCompressed = huffman_->compress(text);
    auto lz77Compressed = lz77_->compress(text);
    
    analysis.huffmanRatio = huffman_->getCompressionRatio(text, huffmanCompressed);
    analysis.lz77Ratio = lz77_->getCompressionRatio(text, lz77Compressed);
    
    if (analysis.huffmanRatio < analysis.lz77Ratio) {
        analysis.bestAlgorithm = Algorithm::HUFFMAN;
        analysis.compressedSize = huffmanCompressed.size();
    } else {
        analysis.bestAlgorithm = Algorithm::LZ77;
        analysis.compressedSize = lz77Compressed.size();
    }
    
    analysis.compressionRatio = static_cast<double>(analysis.compressedSize) / analysis.originalSize;
    
    return analysis;
}

void CompressionManager::printCompressionReport(const std::string& text) {
    auto analysis = analyzeCompression(text);
    
    std::cout << "Compression Analysis Report:\n";
    std::cout << "Original size: " << analysis.originalSize << " bytes\n";
    std::cout << "Huffman ratio: " << std::fixed << std::setprecision(3) << analysis.huffmanRatio << "\n";
    std::cout << "LZ77 ratio: " << std::fixed << std::setprecision(3) << analysis.lz77Ratio << "\n";
    std::cout << "Best algorithm: " << (analysis.bestAlgorithm == Algorithm::HUFFMAN ? "Huffman" : "LZ77") << "\n";
    std::cout << "Compressed size: " << analysis.compressedSize << " bytes\n";
    std::cout << "Final compression ratio: " << std::fixed << std::setprecision(3) << analysis.compressionRatio << "\n";
}

CompressionManager::Algorithm CompressionManager::selectBestAlgorithm(const std::string& text) {
    auto analysis = analyzeCompression(text);
    return analysis.bestAlgorithm;
}

std::vector<uint8_t> CompressionManager::addAlgorithmHeader(const std::vector<uint8_t>& data, Algorithm algorithm) {
    std::vector<uint8_t> result;
    result.push_back(static_cast<uint8_t>(algorithm));
    result.insert(result.end(), data.begin(), data.end());
    return result;
}

CompressionManager::Algorithm CompressionManager::readAlgorithmHeader(const std::vector<uint8_t>& data, size_t& index) {
    if (data.empty()) return Algorithm::HUFFMAN;
    
    Algorithm algorithm = static_cast<Algorithm>(data[0]);
    index = 1;
    return algorithm;
}

} // namespace TextFlow
