#pragma once

#include <string>
#include <vector>
#include <memory>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

namespace TextFlow {

// NLP service response structures
struct GrammarResponse {
    std::string corrected_text;
    std::vector<nlohmann::json> errors;
    std::vector<std::string> suggestions;
};

struct SummarizationResponse {
    std::string summary;
    int original_length;
    int summary_length;
    double compression_ratio;
    std::vector<std::string> key_sentences;
};

struct NERResponse {
    std::vector<nlohmann::json> entities;
    std::vector<std::string> entity_types;
    std::map<std::string, int> entity_counts;
};

struct PredictionResponse {
    std::vector<std::string> predictions;
    std::vector<double> probabilities;
    std::string context;
};

struct SentimentResponse {
    std::string sentiment;
    double confidence;
    std::map<std::string, double> scores;
    std::vector<nlohmann::json> breakdown;
};

struct ReadabilityResponse {
    std::map<std::string, double> scores;
    std::string grade_level;
    std::vector<std::string> recommendations;
    std::string complexity;
};

class NLPClient {
public:
    NLPClient(const std::string& baseUrl = "http://localhost:8000");
    ~NLPClient();
    
    // Connection management
    bool connect();
    void disconnect();
    bool isConnected() const { return connected_; }
    
    // Grammar services
    GrammarResponse checkGrammar(const std::string& text, const std::string& language = "en", bool autoCorrect = false);
    std::string correctGrammar(const std::string& text, const std::string& language = "en");
    
    // Summarization services
    SummarizationResponse summarize(const std::string& text, int maxLength = 100, 
                                  int minLength = 20, const std::string& method = "extractive");
    std::vector<std::string> extractKeywords(const std::string& text, int maxKeywords = 10);
    
    // Named Entity Recognition
    NERResponse extractEntities(const std::string& text, const std::vector<std::string>& entityTypes = {});
    nlohmann::json classifyEntities(const std::string& text);
    
    // Text prediction
    PredictionResponse predictNextWords(const std::string& text, int maxPredictions = 5, int contextLength = 50);
    std::string completeText(const std::string& text);
    
    // Sentiment analysis
    SentimentResponse analyzeSentiment(const std::string& text, const std::string& granularity = "sentence");
    nlohmann::json detectEmotions(const std::string& text);
    
    // Readability analysis
    ReadabilityResponse analyzeReadability(const std::string& text, const std::vector<std::string>& metrics = {});
    std::vector<std::string> getReadabilitySuggestions(const std::string& text);
    
    // Batch processing
    nlohmann::json batchProcess(const std::string& text, const std::vector<std::string>& tasks);
    
    // Health check
    bool healthCheck();

private:
    std::string baseUrl_;
    bool connected_;
    CURL* curl_;
    
    // HTTP request helpers
    std::string makeRequest(const std::string& endpoint, const nlohmann::json& data);
    std::string makeGetRequest(const std::string& endpoint);
    
    // Response parsing
    nlohmann::json parseResponse(const std::string& response);
    GrammarResponse parseGrammarResponse(const nlohmann::json& json);
    SummarizationResponse parseSummarizationResponse(const nlohmann::json& json);
    NERResponse parseNERResponse(const nlohmann::json& json);
    PredictionResponse parsePredictionResponse(const nlohmann::json& json);
    SentimentResponse parseSentimentResponse(const nlohmann::json& json);
    ReadabilityResponse parseReadabilityResponse(const nlohmann::json& json);
    
    // Error handling
    void handleError(const std::string& operation, const std::string& error);
    
    // cURL callback
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s);
};

// NLP integration manager
class NLPIntegrationManager {
public:
    NLPIntegrationManager();
    ~NLPIntegrationManager() = default;
    
    // Initialize NLP services
    bool initialize(const std::string& pythonServiceUrl = "http://localhost:8000");
    void shutdown();
    
    // Text processing pipeline
    struct ProcessedText {
        std::string original_text;
        std::string corrected_text;
        std::string summary;
        std::vector<std::string> keywords;
        std::vector<nlohmann::json> entities;
        std::string sentiment;
        double readability_score;
        std::vector<std::string> suggestions;
    };
    
    ProcessedText processText(const std::string& text, bool enableGrammar = true, 
                            bool enableSummarization = true, bool enableNER = true,
                            bool enableSentiment = true, bool enableReadability = true);
    
    // Real-time features
    std::vector<std::string> getRealTimeSuggestions(const std::string& text, int cursorPosition);
    std::string getAutoComplete(const std::string& text, int cursorPosition);
    
    // Batch processing
    std::vector<ProcessedText> processBatch(const std::vector<std::string>& texts);
    
    // Service status
    bool isServiceAvailable() const;
    std::string getServiceStatus() const;
    
    // Configuration
    void setTimeout(int seconds);
    void setRetryCount(int count);
    void enableCaching(bool enable);

private:
    std::unique_ptr<NLPClient> client_;
    bool initialized_;
    int timeout_;
    int retryCount_;
    bool cachingEnabled_;
    
    // Caching
    std::map<std::string, ProcessedText> cache_;
    void addToCache(const std::string& key, const ProcessedText& result);
    std::optional<ProcessedText> getFromCache(const std::string& key);
    std::string generateCacheKey(const std::string& text, bool grammar, bool summary, 
                               bool ner, bool sentiment, bool readability);
};

} // namespace TextFlow
