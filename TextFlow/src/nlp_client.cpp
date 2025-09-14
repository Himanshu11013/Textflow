#include "nlp_client.h"
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace TextFlow {

NLPClient::NLPClient(const std::string& baseUrl) 
    : baseUrl_(baseUrl), connected_(false), curl_(nullptr) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl_ = curl_easy_init();
}

NLPClient::~NLPClient() {
    if (curl_) {
        curl_easy_cleanup(curl_);
    }
    curl_global_cleanup();
}

bool NLPClient::connect() {
    if (!curl_) {
        return false;
    }
    
    // Test connection with health check
    connected_ = healthCheck();
    return connected_;
}

void NLPClient::disconnect() {
    connected_ = false;
}

GrammarResponse NLPClient::checkGrammar(const std::string& text, const std::string& language, bool autoCorrect) {
    if (!connected_) {
        return {"", {}, {}};
    }
    
    try {
        nlohmann::json request = {
            {"text", text},
            {"language", language},
            {"auto_correct", autoCorrect}
        };
        
        std::string response = makeRequest("/grammar/check", request);
        nlohmann::json jsonResponse = parseResponse(response);
        return parseGrammarResponse(jsonResponse);
    } catch (const std::exception& e) {
        handleError("Grammar check", e.what());
        return {"", {}, {}};
    }
}

std::string NLPClient::correctGrammar(const std::string& text, const std::string& language) {
    if (!connected_) {
        return text;
    }
    
    try {
        nlohmann::json request = {
            {"text", text},
            {"language", language}
        };
        
        std::string response = makeRequest("/grammar/correct", request);
        nlohmann::json jsonResponse = parseResponse(response);
        
        if (jsonResponse.contains("corrected_text")) {
            return jsonResponse["corrected_text"];
        }
        
        return text;
    } catch (const std::exception& e) {
        handleError("Grammar correction", e.what());
        return text;
    }
}

SummarizationResponse NLPClient::summarize(const std::string& text, int maxLength, int minLength, const std::string& method) {
    if (!connected_) {
        return {"", 0, 0, 0.0, {}};
    }
    
    try {
        nlohmann::json request = {
            {"text", text},
            {"max_length", maxLength},
            {"min_length", minLength},
            {"method", method}
        };
        
        std::string response = makeRequest("/summarize", request);
        nlohmann::json jsonResponse = parseResponse(response);
        return parseSummarizationResponse(jsonResponse);
    } catch (const std::exception& e) {
        handleError("Summarization", e.what());
        return {"", 0, 0, 0.0, {}};
    }
}

std::vector<std::string> NLPClient::extractKeywords(const std::string& text, int maxKeywords) {
    if (!connected_) {
        return {};
    }
    
    try {
        nlohmann::json request = {
            {"text", text}
        };
        
        std::string response = makeRequest("/summarize/keywords", request);
        nlohmann::json jsonResponse = parseResponse(response);
        
        if (jsonResponse.contains("keywords")) {
            return jsonResponse["keywords"].get<std::vector<std::string>>();
        }
        
        return {};
    } catch (const std::exception& e) {
        handleError("Keyword extraction", e.what());
        return {};
    }
}

NERResponse NLPClient::extractEntities(const std::string& text, const std::vector<std::string>& entityTypes) {
    if (!connected_) {
        return {{}, {}, {}};
    }
    
    try {
        nlohmann::json request = {
            {"text", text},
            {"entities", entityTypes}
        };
        
        std::string response = makeRequest("/ner/extract", request);
        nlohmann::json jsonResponse = parseResponse(response);
        return parseNERResponse(jsonResponse);
    } catch (const std::exception& e) {
        handleError("Entity extraction", e.what());
        return {{}, {}, {}};
    }
}

nlohmann::json NLPClient::classifyEntities(const std::string& text) {
    if (!connected_) {
        return {};
    }
    
    try {
        nlohmann::json request = {
            {"text", text}
        };
        
        std::string response = makeRequest("/ner/classify", request);
        return parseResponse(response);
    } catch (const std::exception& e) {
        handleError("Entity classification", e.what());
        return {};
    }
}

PredictionResponse NLPClient::predictNextWords(const std::string& text, int maxPredictions, int contextLength) {
    if (!connected_) {
        return {{}, {}, ""};
    }
    
    try {
        nlohmann::json request = {
            {"text", text},
            {"max_predictions", maxPredictions},
            {"context_length", contextLength}
        };
        
        std::string response = makeRequest("/predict/next", request);
        nlohmann::json jsonResponse = parseResponse(response);
        return parsePredictionResponse(jsonResponse);
    } catch (const std::exception& e) {
        handleError("Text prediction", e.what());
        return {{}, {}, ""};
    }
}

std::string NLPClient::completeText(const std::string& text) {
    if (!connected_) {
        return text;
    }
    
    try {
        nlohmann::json request = {
            {"text", text}
        };
        
        std::string response = makeRequest("/predict/complete", request);
        nlohmann::json jsonResponse = parseResponse(response);
        
        if (jsonResponse.contains("completed_text")) {
            return jsonResponse["completed_text"];
        }
        
        return text;
    } catch (const std::exception& e) {
        handleError("Text completion", e.what());
        return text;
    }
}

SentimentResponse NLPClient::analyzeSentiment(const std::string& text, const std::string& granularity) {
    if (!connected_) {
        return {"neutral", 0.5, {}, {}};
    }
    
    try {
        nlohmann::json request = {
            {"text", text},
            {"granularity", granularity}
        };
        
        std::string response = makeRequest("/sentiment/analyze", request);
        nlohmann::json jsonResponse = parseResponse(response);
        return parseSentimentResponse(jsonResponse);
    } catch (const std::exception& e) {
        handleError("Sentiment analysis", e.what());
        return {"neutral", 0.5, {}, {}};
    }
}

nlohmann::json NLPClient::detectEmotions(const std::string& text) {
    if (!connected_) {
        return {};
    }
    
    try {
        nlohmann::json request = {
            {"text", text}
        };
        
        std::string response = makeRequest("/sentiment/emotions", request);
        return parseResponse(response);
    } catch (const std::exception& e) {
        handleError("Emotion detection", e.what());
        return {};
    }
}

ReadabilityResponse NLPClient::analyzeReadability(const std::string& text, const std::vector<std::string>& metrics) {
    if (!connected_) {
        return {{}, "", {}, ""};
    }
    
    try {
        nlohmann::json request = {
            {"text", text},
            {"metrics", metrics}
        };
        
        std::string response = makeRequest("/readability/analyze", request);
        nlohmann::json jsonResponse = parseResponse(response);
        return parseReadabilityResponse(jsonResponse);
    } catch (const std::exception& e) {
        handleError("Readability analysis", e.what());
        return {{}, "", {}, ""};
    }
}

std::vector<std::string> NLPClient::getReadabilitySuggestions(const std::string& text) {
    if (!connected_) {
        return {};
    }
    
    try {
        nlohmann::json request = {
            {"text", text}
        };
        
        std::string response = makeRequest("/readability/improve", request);
        nlohmann::json jsonResponse = parseResponse(response);
        
        if (jsonResponse.contains("suggestions")) {
            return jsonResponse["suggestions"].get<std::vector<std::string>>();
        }
        
        return {};
    } catch (const std::exception& e) {
        handleError("Readability suggestions", e.what());
        return {};
    }
}

nlohmann::json NLPClient::batchProcess(const std::string& text, const std::vector<std::string>& tasks) {
    if (!connected_) {
        return {};
    }
    
    try {
        nlohmann::json request = {
            {"text", text},
            {"tasks", tasks}
        };
        
        std::string response = makeRequest("/batch/process", request);
        return parseResponse(response);
    } catch (const std::exception& e) {
        handleError("Batch processing", e.what());
        return {};
    }
}

bool NLPClient::healthCheck() {
    try {
        std::string response = makeGetRequest("/health");
        nlohmann::json jsonResponse = parseResponse(response);
        return jsonResponse.contains("status") && jsonResponse["status"] == "healthy";
    } catch (const std::exception& e) {
        return false;
    }
}

std::string NLPClient::makeRequest(const std::string& endpoint, const nlohmann::json& data) {
    if (!curl_) {
        throw std::runtime_error("cURL not initialized");
    }
    
    std::string url = baseUrl_ + endpoint;
    std::string jsonData = data.dump();
    std::string response;
    
    curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, jsonData.c_str());
    curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, jsonData.length());
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response);
    
    // Set headers
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers);
    
    CURLcode res = curl_easy_perform(curl_);
    curl_slist_free_all(headers);
    
    if (res != CURLE_OK) {
        throw std::runtime_error("cURL request failed: " + std::string(curl_easy_strerror(res)));
    }
    
    return response;
}

std::string NLPClient::makeGetRequest(const std::string& endpoint) {
    if (!curl_) {
        throw std::runtime_error("cURL not initialized");
    }
    
    std::string url = baseUrl_ + endpoint;
    std::string response;
    
    curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response);
    
    CURLcode res = curl_easy_perform(curl_);
    
    if (res != CURLE_OK) {
        throw std::runtime_error("cURL GET request failed: " + std::string(curl_easy_strerror(res)));
    }
    
    return response;
}

nlohmann::json NLPClient::parseResponse(const std::string& response) {
    try {
        return nlohmann::json::parse(response);
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to parse JSON response: " + std::string(e.what()));
    }
}

GrammarResponse NLPClient::parseGrammarResponse(const nlohmann::json& json) {
    GrammarResponse result;
    
    if (json.contains("corrected_text")) {
        result.corrected_text = json["corrected_text"];
    }
    
    if (json.contains("errors")) {
        result.errors = json["errors"];
    }
    
    if (json.contains("suggestions")) {
        result.suggestions = json["suggestions"].get<std::vector<std::string>>();
    }
    
    return result;
}

SummarizationResponse NLPClient::parseSummarizationResponse(const nlohmann::json& json) {
    SummarizationResponse result;
    
    if (json.contains("summary")) {
        result.summary = json["summary"];
    }
    
    if (json.contains("original_length")) {
        result.original_length = json["original_length"];
    }
    
    if (json.contains("summary_length")) {
        result.summary_length = json["summary_length"];
    }
    
    if (json.contains("compression_ratio")) {
        result.compression_ratio = json["compression_ratio"];
    }
    
    if (json.contains("key_sentences")) {
        result.key_sentences = json["key_sentences"].get<std::vector<std::string>>();
    }
    
    return result;
}

NERResponse NLPClient::parseNERResponse(const nlohmann::json& json) {
    NERResponse result;
    
    if (json.contains("entities")) {
        result.entities = json["entities"];
    }
    
    if (json.contains("entity_types")) {
        result.entity_types = json["entity_types"].get<std::vector<std::string>>();
    }
    
    if (json.contains("entity_counts")) {
        result.entity_counts = json["entity_counts"].get<std::map<std::string, int>>();
    }
    
    return result;
}

PredictionResponse NLPClient::parsePredictionResponse(const nlohmann::json& json) {
    PredictionResponse result;
    
    if (json.contains("predictions")) {
        result.predictions = json["predictions"].get<std::vector<std::string>>();
    }
    
    if (json.contains("probabilities")) {
        result.probabilities = json["probabilities"].get<std::vector<double>>();
    }
    
    if (json.contains("context")) {
        result.context = json["context"];
    }
    
    return result;
}

SentimentResponse NLPClient::parseSentimentResponse(const nlohmann::json& json) {
    SentimentResponse result;
    
    if (json.contains("sentiment")) {
        result.sentiment = json["sentiment"];
    }
    
    if (json.contains("confidence")) {
        result.confidence = json["confidence"];
    }
    
    if (json.contains("scores")) {
        result.scores = json["scores"].get<std::map<std::string, double>>();
    }
    
    if (json.contains("breakdown")) {
        result.breakdown = json["breakdown"];
    }
    
    return result;
}

ReadabilityResponse NLPClient::parseReadabilityResponse(const nlohmann::json& json) {
    ReadabilityResponse result;
    
    if (json.contains("scores")) {
        result.scores = json["scores"].get<std::map<std::string, double>>();
    }
    
    if (json.contains("grade_level")) {
        result.grade_level = json["grade_level"];
    }
    
    if (json.contains("recommendations")) {
        result.recommendations = json["recommendations"].get<std::vector<std::string>>();
    }
    
    if (json.contains("complexity")) {
        result.complexity = json["complexity"];
    }
    
    return result;
}

void NLPClient::handleError(const std::string& operation, const std::string& error) {
    std::cerr << "NLP Client Error in " << operation << ": " << error << std::endl;
}

size_t NLPClient::WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s) {
    size_t newLength = size * nmemb;
    try {
        s->append((char*)contents, newLength);
        return newLength;
    } catch (std::bad_alloc& e) {
        return 0;
    }
}

// NLPIntegrationManager implementation

NLPIntegrationManager::NLPIntegrationManager() 
    : initialized_(false), timeout_(30), retryCount_(3), cachingEnabled_(false) {}

bool NLPIntegrationManager::initialize(const std::string& pythonServiceUrl) {
    try {
        client_ = std::make_unique<NLPClient>(pythonServiceUrl);
        initialized_ = client_->connect();
        return initialized_;
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize NLP integration: " << e.what() << std::endl;
        return false;
    }
}

void NLPIntegrationManager::shutdown() {
    if (client_) {
        client_->disconnect();
    }
    initialized_ = false;
}

NLPIntegrationManager::ProcessedText NLPIntegrationManager::processText(
    const std::string& text, bool enableGrammar, bool enableSummarization, 
    bool enableNER, bool enableSentiment, bool enableReadability) {
    
    ProcessedText result;
    result.original_text = text;
    
    // Check cache first
    if (cachingEnabled_) {
        std::string cacheKey = generateCacheKey(text, enableGrammar, enableSummarization, 
                                               enableNER, enableSentiment, enableReadability);
        auto cached = getFromCache(cacheKey);
        if (cached) {
            return *cached;
        }
    }
    
    if (!initialized_ || !client_) {
        return result;
    }
    
    try {
        // Grammar correction
        if (enableGrammar) {
            result.corrected_text = client_->correctGrammar(text);
        } else {
            result.corrected_text = text;
        }
        
        // Summarization
        if (enableSummarization) {
            auto summary = client_->summarize(text, 100, 20);
            result.summary = summary.summary;
        }
        
        // Named Entity Recognition
        if (enableNER) {
            auto ner = client_->extractEntities(text);
            for (const auto& entity : ner.entities) {
                result.entities.push_back(entity);
            }
        }
        
        // Sentiment analysis
        if (enableSentiment) {
            auto sentiment = client_->analyzeSentiment(text);
            result.sentiment = sentiment.sentiment;
        }
        
        // Readability analysis
        if (enableReadability) {
            auto readability = client_->analyzeReadability(text);
            result.readability_score = readability.scores.count("flesch_kincaid") ? 
                                     readability.scores["flesch_kincaid"] : 0.0;
            result.suggestions = readability.recommendations;
        }
        
        // Add to cache
        if (cachingEnabled_) {
            std::string cacheKey = generateCacheKey(text, enableGrammar, enableSummarization, 
                                                   enableNER, enableSentiment, enableReadability);
            addToCache(cacheKey, result);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error processing text: " << e.what() << std::endl;
    }
    
    return result;
}

std::vector<std::string> NLPIntegrationManager::getRealTimeSuggestions(const std::string& text, int cursorPosition) {
    if (!initialized_ || !client_) {
        return {};
    }
    
    try {
        // Get text up to cursor position
        std::string context = text.substr(0, cursorPosition);
        
        // Get predictions
        auto predictions = client_->predictNextWords(context, 5, 50);
        return predictions.predictions;
    } catch (const std::exception& e) {
        std::cerr << "Error getting real-time suggestions: " << e.what() << std::endl;
        return {};
    }
}

std::string NLPIntegrationManager::getAutoComplete(const std::string& text, int cursorPosition) {
    if (!initialized_ || !client_) {
        return text;
    }
    
    try {
        return client_->completeText(text);
    } catch (const std::exception& e) {
        std::cerr << "Error getting auto-complete: " << e.what() << std::endl;
        return text;
    }
}

std::vector<NLPIntegrationManager::ProcessedText> NLPIntegrationManager::processBatch(const std::vector<std::string>& texts) {
    std::vector<ProcessedText> results;
    
    for (const auto& text : texts) {
        results.push_back(processText(text));
    }
    
    return results;
}

bool NLPIntegrationManager::isServiceAvailable() const {
    return initialized_ && client_ && client_->isConnected();
}

std::string NLPIntegrationManager::getServiceStatus() const {
    if (!initialized_) {
        return "Not initialized";
    }
    
    if (!client_) {
        return "Client not available";
    }
    
    if (client_->healthCheck()) {
        return "Healthy";
    } else {
        return "Unhealthy";
    }
}

void NLPIntegrationManager::setTimeout(int seconds) {
    timeout_ = seconds;
}

void NLPIntegrationManager::setRetryCount(int count) {
    retryCount_ = count;
}

void NLPIntegrationManager::enableCaching(bool enable) {
    cachingEnabled_ = enable;
}

void NLPIntegrationManager::addToCache(const std::string& key, const ProcessedText& result) {
    cache_[key] = result;
}

std::optional<NLPIntegrationManager::ProcessedText> NLPIntegrationManager::getFromCache(const std::string& key) {
    auto it = cache_.find(key);
    if (it != cache_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::string NLPIntegrationManager::generateCacheKey(const std::string& text, bool grammar, bool summary, 
                                                   bool ner, bool sentiment, bool readability) {
    std::ostringstream oss;
    oss << text << "_" << grammar << "_" << summary << "_" << ner << "_" << sentiment << "_" << readability;
    return oss.str();
}

} // namespace TextFlow
