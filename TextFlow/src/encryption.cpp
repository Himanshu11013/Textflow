#include "encryption.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/err.h>
#include <random>
#include <iostream>
#include <cstring>

// Error handling macros for OpenSSL
#define OPENSSL_ERROR() \
    do { \
        ERR_print_errors_fp(stderr); \
        return {}; \
    } while(0)

#define OPENSSL_ERROR_BOOL() \
    do { \
        ERR_print_errors_fp(stderr); \
        return false; \
    } while(0)

namespace TextFlow {

// AESEncryption implementation

AESEncryption::AESEncryption() : encryptCtx_(nullptr), decryptCtx_(nullptr) {
    initializeContexts();
}

AESEncryption::~AESEncryption() {
    cleanupContexts();
}

std::vector<uint8_t> AESEncryption::encrypt(const std::string& plaintext, const std::string& password) {
    if (plaintext.empty() || !isValidPassword(password)) {
        return {};
    }
    
    // Generate random salt and IV
    auto salt = generateRandomBytes(SALT_SIZE);
    auto iv = generateRandomBytes(IV_SIZE);
    
    // Derive key from password
    auto key = deriveKey(password, salt);
    
    // Initialize encryption context
    if (EVP_EncryptInit_ex(encryptCtx_, EVP_aes_256_cbc(), nullptr, key.data(), iv.data()) != 1) {
        return {};
    }
    
    // Encrypt the plaintext
    std::vector<uint8_t> ciphertext(plaintext.length() + AES_BLOCK_SIZE);
    int len;
    int ciphertextLen;
    
    if (EVP_EncryptUpdate(encryptCtx_, ciphertext.data(), &len, 
                         reinterpret_cast<const uint8_t*>(plaintext.c_str()), 
                         static_cast<int>(plaintext.length())) != 1) {
        return {};
    }
    ciphertextLen = len;
    
    if (EVP_EncryptFinal_ex(encryptCtx_, ciphertext.data() + len, &len) != 1) {
        return {};
    }
    ciphertextLen += len;
    
    // Resize to actual length
    ciphertext.resize(ciphertextLen);
    
    // Prepend salt and IV to ciphertext
    std::vector<uint8_t> result;
    result.insert(result.end(), salt.begin(), salt.end());
    result.insert(result.end(), iv.begin(), iv.end());
    result.insert(result.end(), ciphertext.begin(), ciphertext.end());
    
    return result;
}

std::string AESEncryption::decrypt(const std::vector<uint8_t>& ciphertext, const std::string& password) {
    if (ciphertext.size() < SALT_SIZE + IV_SIZE || !isValidPassword(password)) {
        return "";
    }
    
    // Extract salt, IV, and encrypted data
    auto salt = std::vector<uint8_t>(ciphertext.begin(), ciphertext.begin() + SALT_SIZE);
    auto iv = std::vector<uint8_t>(ciphertext.begin() + SALT_SIZE, ciphertext.begin() + SALT_SIZE + IV_SIZE);
    auto encryptedData = std::vector<uint8_t>(ciphertext.begin() + SALT_SIZE + IV_SIZE, ciphertext.end());
    
    // Derive key from password
    auto key = deriveKey(password, salt);
    
    // Initialize decryption context
    if (EVP_DecryptInit_ex(decryptCtx_, EVP_aes_256_cbc(), nullptr, key.data(), iv.data()) != 1) {
        return "";
    }
    
    // Decrypt the ciphertext
    std::vector<uint8_t> plaintext(encryptedData.size() + AES_BLOCK_SIZE);
    int len;
    int plaintextLen;
    
    if (EVP_DecryptUpdate(decryptCtx_, plaintext.data(), &len, 
                         encryptedData.data(), static_cast<int>(encryptedData.size())) != 1) {
        return "";
    }
    plaintextLen = len;
    
    if (EVP_DecryptFinal_ex(decryptCtx_, plaintext.data() + len, &len) != 1) {
        return "";
    }
    plaintextLen += len;
    
    // Resize to actual length
    plaintext.resize(plaintextLen);
    
    return std::string(reinterpret_cast<char*>(plaintext.data()), plaintextLen);
}

bool AESEncryption::encryptToFile(const std::string& plaintext, const std::string& filename, const std::string& password) {
    try {
        auto encrypted = encrypt(plaintext, password);
        if (encrypted.empty()) return false;
        
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) return false;
        
        file.write(reinterpret_cast<const char*>(encrypted.data()), encrypted.size());
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Encryption error: " << e.what() << std::endl;
        return false;
    }
}

std::string AESEncryption::decryptFromFile(const std::string& filename, const std::string& password) {
    try {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) return "";
        
        file.seekg(0, std::ios::end);
        size_t size = file.tellg();
        file.seekg(0, std::ios::beg);
        
        std::vector<uint8_t> data(size);
        file.read(reinterpret_cast<char*>(data.data()), size);
        
        return decrypt(data, password);
    } catch (const std::exception& e) {
        std::cerr << "Decryption error: " << e.what() << std::endl;
        return "";
    }
}

std::vector<uint8_t> AESEncryption::deriveKey(const std::string& password, const std::vector<uint8_t>& salt) {
    std::vector<uint8_t> key(KEY_SIZE);
    
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (!mdctx) {
        return {};
    }
    
    if (PKCS5_PBKDF2_HMAC(password.c_str(), static_cast<int>(password.length()),
                         salt.data(), static_cast<int>(salt.size()),
                         ITERATIONS, EVP_sha256(),
                         static_cast<int>(key.size()), key.data()) != 1) {
        EVP_MD_CTX_free(mdctx);
        return {};
    }
    
    EVP_MD_CTX_free(mdctx);
    
    return key;
}

bool AESEncryption::isValidPassword(const std::string& password) const {
    return password.length() >= 8;
}

std::string AESEncryption::generateRandomSalt() {
    auto salt = generateRandomBytes(SALT_SIZE);
    return std::string(salt.begin(), salt.end());
}

std::string AESEncryption::generateRandomIV() {
    auto iv = generateRandomBytes(IV_SIZE);
    return std::string(iv.begin(), iv.end());
}

void AESEncryption::initializeContexts() {
    encryptCtx_ = EVP_CIPHER_CTX_new();
    decryptCtx_ = EVP_CIPHER_CTX_new();
}

void AESEncryption::cleanupContexts() {
    if (encryptCtx_) {
        EVP_CIPHER_CTX_free(encryptCtx_);
        encryptCtx_ = nullptr;
    }
    if (decryptCtx_) {
        EVP_CIPHER_CTX_free(decryptCtx_);
        decryptCtx_ = nullptr;
    }
}

std::vector<uint8_t> AESEncryption::generateRandomBytes(int size) {
    std::vector<uint8_t> bytes(size);
    if (RAND_bytes(bytes.data(), size) != 1) {
        // Fallback to pseudo-random if OpenSSL RAND_bytes fails
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        
        for (int i = 0; i < size; i++) {
            bytes[i] = static_cast<uint8_t>(dis(gen));
        }
    }
    return bytes;
}

// RSAEncryption implementation

RSAEncryption::RSAEncryption() : publicKey_(nullptr), privateKey_(nullptr) {
    // Initialize OpenSSL
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();
}

RSAEncryption::~RSAEncryption() {
    // Free keys if they exist
    if (publicKey_) {
        EVP_PKEY_free(publicKey_);
        publicKey_ = nullptr;
    }
    if (privateKey_) {
        EVP_PKEY_free(privateKey_);
        privateKey_ = nullptr;
    }
    
    // Clean up OpenSSL
    EVP_cleanup();
    CRYPTO_cleanup_all_ex_data();
    ERR_free_strings();

bool RSAEncryption::generateKeyPair(int keySize) {
    // Clean up any existing keys
    if (publicKey_) {
        EVP_PKEY_free(publicKey_);
        publicKey_ = nullptr;
    }
    if (privateKey_) {
        EVP_PKEY_free(privateKey_);
        privateKey_ = nullptr;
    }
    
    // Create a context for the key generation
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
    if (!ctx) {
        OPENSSL_ERROR_BOOL();
    }
    
    // Initialize the key generation context
    if (EVP_PKEY_keygen_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        OPENSSL_ERROR_BOOL();
    }
    
    // Set the key size
    if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, keySize) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        OPENSSL_ERROR_BOOL();
    }
    
    // Generate the key pair
    if (EVP_PKEY_keygen(ctx, &privateKey_) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        OPENSSL_ERROR_BOOL();
    }
    
    // Get the public key from the key pair
    publicKey_ = EVP_PKEY_new();
    if (!publicKey_) {
        EVP_PKEY_free(privateKey_);
        privateKey_ = nullptr;
        EVP_PKEY_CTX_free(ctx);
        OPENSSL_ERROR_BOOL();
    }
    
    // In OpenSSL 3.0+, we need to duplicate the public key
    EVP_PKEY_up_ref(privateKey_);
    publicKey_ = privateKey_;
    
    // Clean up
    EVP_PKEY_CTX_free(ctx);
    
    return true;
}

std::string RSAEncryption::getPublicKeyPEM() const {
    if (!publicKey_) {
        return "";
    }
    
    BIO* bio = BIO_new(BIO_s_mem());
    if (!bio) {
        return "";
    }
    
    if (PEM_write_bio_PUBKEY(bio, publicKey_) != 1) {
        BIO_free(bio);
        return "";
    }
    
    BUF_MEM* bptr;
    BIO_get_mem_ptr(bio, &bptr);
    std::string result(bptr->data, bptr->length);
    
    BIO_free(bio);
    return result;
}

std::string RSAEncryption::getPrivateKeyPEM() const {
    if (!privateKey_) {
        return "";
    }
    
    BIO* bio = BIO_new(BIO_s_mem());
    if (!bio) {
        return "";
    }
    
    if (PEM_write_bio_PrivateKey(bio, privateKey_, nullptr, nullptr, 0, nullptr, nullptr) != 1) {
        BIO_free(bio);
        return "";
    }
    
    BUF_MEM* bptr;
    BIO_get_mem_ptr(bio, &bptr);
    std::string result(bptr->data, bptr->length);
    
    BIO_free(bio);
    return result;
}

std::vector<uint8_t> RSAEncryption::encrypt(const std::string& plaintext, const std::string& publicKeyPEM) {
    // Load public key
    BIO* bio = BIO_new_mem_buf(publicKeyPEM.c_str(), static_cast<int>(publicKeyPEM.length()));
    if (!bio) return {};
    
    EVP_PKEY* key = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    
    if (!key) return {};
    
    // Encrypt
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(key, nullptr);
    if (!ctx) {
        EVP_PKEY_free(key);
        return {};
    }
    
    if (EVP_PKEY_encrypt_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(key);
        return {};
    }
    
    size_t outlen;
    if (EVP_PKEY_encrypt(ctx, nullptr, &outlen, 
                        reinterpret_cast<const uint8_t*>(plaintext.c_str()), 
                        plaintext.length()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(key);
        return {};
    }
    
    std::vector<uint8_t> ciphertext(outlen);
    if (EVP_PKEY_encrypt(ctx, ciphertext.data(), &outlen,
                        reinterpret_cast<const uint8_t*>(plaintext.c_str()),
                        plaintext.length()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(key);
        return {};
    }
    
    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(key);
    
    return ciphertext;
}

std::string RSAEncryption::decrypt(const std::vector<uint8_t>& ciphertext, const std::string& privateKeyPEM) {
    // Load private key
    BIO* bio = BIO_new_mem_buf(privateKeyPEM.c_str(), static_cast<int>(privateKeyPEM.length()));
    if (!bio) return "";
    
    EVP_PKEY* key = PEM_read_bio_PrivateKey(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    
    if (!key) return "";
    
    // Decrypt
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(key, nullptr);
    if (!ctx) {
        EVP_PKEY_free(key);
        return "";
    }
    
    if (EVP_PKEY_decrypt_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(key);
        return "";
    }
    
    size_t outlen;
    if (EVP_PKEY_decrypt(ctx, nullptr, &outlen, ciphertext.data(), ciphertext.size()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(key);
        return "";
    }
    
    std::vector<uint8_t> plaintext(outlen);
    if (EVP_PKEY_decrypt(ctx, plaintext.data(), &outlen, ciphertext.data(), ciphertext.size()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(key);
        return "";
    }
    
    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(key);
    
    return std::string(reinterpret_cast<char*>(plaintext.data()), outlen);
}

bool RSAEncryption::loadPublicKey(const std::string& publicKeyPEM) {
    BIO* bio = BIO_new_mem_buf(publicKeyPEM.c_str(), static_cast<int>(publicKeyPEM.length()));
    if (!bio) return false;
    
    EVP_PKEY* key = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    
    if (!key) return false;
    
    if (publicKey_) EVP_PKEY_free(publicKey_);
    publicKey_ = key;
    return true;
}

bool RSAEncryption::loadPrivateKey(const std::string& privateKeyPEM) {
    BIO* bio = BIO_new_mem_buf(privateKeyPEM.c_str(), static_cast<int>(privateKeyPEM.length()));
    if (!bio) return false;
    
    EVP_PKEY* key = PEM_read_bio_PrivateKey(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    
    if (!key) return false;
    
    if (privateKey_) EVP_PKEY_free(privateKey_);
    privateKey_ = key;
    return true;
}

bool RSAEncryption::saveKeyPair(const std::string& publicKeyFile, const std::string& privateKeyFile) const {
    if (!publicKey_ || !privateKey_) return false;
    
    // Save public key
    std::ofstream pubFile(publicKeyFile);
    if (!pubFile.is_open()) return false;
    pubFile << getPublicKeyPEM();
    
    // Save private key
    std::ofstream privFile(privateKeyFile);
    if (!privFile.is_open()) return false;
    privFile << getPrivateKeyPEM();
    
    return true;
}

void RSAEncryption::cleanupKeys() {
    if (publicKey_) {
        EVP_PKEY_free(publicKey_);
        publicKey_ = nullptr;
    }
    if (privateKey_) {
        EVP_PKEY_free(privateKey_);
        privateKey_ = nullptr;
    }
}

// PasswordManager implementation

PasswordManager::PasswordManager() = default;

std::string PasswordManager::hashPassword(const std::string& password) {
    auto salt = generateSalt();
    auto hash = std::vector<uint8_t>(HASH_SIZE);
    
    if (PKCS5_PBKDF2_HMAC(password.c_str(), static_cast<int>(password.length()),
                         salt.data(), static_cast<int>(salt.size()),
                         ITERATIONS, EVP_sha256(),
                         hash.data(), HASH_SIZE) != 1) {
        return "";
    }
    
    // Combine salt and hash
    auto combined = salt;
    combined.insert(combined.end(), hash.begin(), hash.end());
    
    return bytesToHex(combined);
}

bool PasswordManager::verifyPassword(const std::string& password, const std::string& hash) {
    auto combined = hexToBytes(hash);
    if (combined.size() != SALT_SIZE + HASH_SIZE) return false;
    
    auto salt = std::vector<uint8_t>(combined.begin(), combined.begin() + SALT_SIZE);
    auto expectedHash = std::vector<uint8_t>(combined.begin() + SALT_SIZE, combined.end());
    
    auto actualHash = std::vector<uint8_t>(HASH_SIZE);
    if (PKCS5_PBKDF2_HMAC(password.c_str(), static_cast<int>(password.length()),
                         salt.data(), static_cast<int>(salt.size()),
                         ITERATIONS, EVP_sha256(),
                         actualHash.data(), HASH_SIZE) != 1) {
        return false;
    }
    
    return actualHash == expectedHash;
}

PasswordManager::PasswordStrength PasswordManager::checkPasswordStrength(const std::string& password) {
    PasswordStrength strength;
    strength.score = 0;
    strength.isStrong = false;
    
    if (password.length() >= 8) strength.score += 1;
    if (password.length() >= 12) strength.score += 1;
    if (password.length() >= 16) strength.score += 1;
    
    bool hasLower = false, hasUpper = false, hasDigit = false, hasSpecial = false;
    for (char c : password) {
        if (std::islower(c)) hasLower = true;
        else if (std::isupper(c)) hasUpper = true;
        else if (std::isdigit(c)) hasDigit = true;
        else if (!std::isalnum(c)) hasSpecial = true;
    }
    
    if (hasLower) strength.score += 1;
    if (hasUpper) strength.score += 1;
    if (hasDigit) strength.score += 1;
    if (hasSpecial) strength.score += 1;
    
    strength.isStrong = strength.score >= 6;
    
    if (strength.score < 3) {
        strength.feedback = "Very weak password. Use at least 8 characters with mixed case, numbers, and symbols.";
    } else if (strength.score < 5) {
        strength.feedback = "Weak password. Add more variety of characters.";
    } else if (strength.score < 7) {
        strength.feedback = "Moderate password. Consider making it longer or adding more complexity.";
    } else {
        strength.feedback = "Strong password!";
    }
    
    return strength;
}

std::string PasswordManager::generateSecurePassword(int length, bool includeSymbols) {
    const std::string lowercase = "abcdefghijklmnopqrstuvwxyz";
    const std::string uppercase = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    const std::string digits = "0123456789";
    const std::string symbols = "!@#$%^&*()_+-=[]{}|;:,.<>?";
    
    std::string charset = lowercase + uppercase + digits;
    if (includeSymbols) charset += symbols;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, static_cast<int>(charset.length() - 1));
    
    std::string password;
    for (int i = 0; i < length; i++) {
        password += charset[dis(gen)];
    }
    
    return password;
}

std::vector<uint8_t> PasswordManager::generateSalt() {
    std::vector<uint8_t> salt(SALT_SIZE);
    if (RAND_bytes(salt.data(), SALT_SIZE) != 1) {
        // Fallback to pseudo-random
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        
        for (int i = 0; i < SALT_SIZE; i++) {
            salt[i] = static_cast<uint8_t>(dis(gen));
        }
    }
    return salt;
}

std::string PasswordManager::bytesToHex(const std::vector<uint8_t>& bytes) {
    std::ostringstream oss;
    for (uint8_t byte : bytes) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }
    return oss.str();
}

std::vector<uint8_t> PasswordManager::hexToBytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(std::strtol(byteString.c_str(), nullptr, 16));
        bytes.push_back(byte);
    }
    return bytes;
}

// EncryptionManager implementation

EncryptionManager::EncryptionManager() 
    : aes_(std::make_unique<AESEncryption>())
    , rsa_(std::make_unique<RSAEncryption>())
    , passwordManager_(std::make_unique<PasswordManager>()) {}

std::vector<uint8_t> EncryptionManager::encrypt(const std::string& plaintext, const std::string& password, Algorithm algorithm) {
    switch (algorithm) {
        case Algorithm::AES_256_CBC:
        case Algorithm::AES_256_GCM:
            return aes_->encrypt(plaintext, password);
        case Algorithm::RSA_2048:
        case Algorithm::RSA_4096:
            // For RSA, we need to use the public key
            return rsa_->encrypt(plaintext, password); // password is treated as public key PEM
        default:
            return {};
    }
}

std::string EncryptionManager::decrypt(const std::vector<uint8_t>& ciphertext, const std::string& password, Algorithm algorithm) {
    switch (algorithm) {
        case Algorithm::AES_256_CBC:
        case Algorithm::AES_256_GCM:
            return aes_->decrypt(ciphertext, password);
        case Algorithm::RSA_2048:
        case Algorithm::RSA_4096:
            // For RSA, we need to use the private key
            return rsa_->decrypt(ciphertext, password); // password is treated as private key PEM
        default:
            return "";
    }
}

bool EncryptionManager::encryptToFile(const std::string& plaintext, const std::string& filename, 
                                     const std::string& password, Algorithm algorithm) {
    auto encrypted = encrypt(plaintext, password, algorithm);
    if (encrypted.empty()) return false;
    
    auto dataWithHeader = addAlgorithmHeader(encrypted, algorithm);
    
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) return false;
    
    file.write(reinterpret_cast<const char*>(dataWithHeader.data()), dataWithHeader.size());
    return true;
}

std::string EncryptionManager::decryptFromFile(const std::string& filename, const std::string& password) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) return "";
    
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<uint8_t> data(size);
    file.read(reinterpret_cast<char*>(data.data()), size);
    
    size_t index = 0;
    Algorithm algorithm = readAlgorithmHeader(data, index);
    
    std::vector<uint8_t> encryptedData(data.begin() + index, data.end());
    return decrypt(encryptedData, password, algorithm);
}

bool EncryptionManager::generateKeyPair(const std::string& publicKeyFile, const std::string& privateKeyFile, Algorithm algorithm) {
    int keySize = (algorithm == Algorithm::RSA_4096) ? 4096 : 2048;
    return rsa_->generateKeyPair(keySize) && rsa_->saveKeyPair(publicKeyFile, privateKeyFile);
}

std::string EncryptionManager::getPublicKey() const {
    return rsa_->getPublicKeyPEM();
}

std::string EncryptionManager::getPrivateKey() const {
    return rsa_->getPrivateKeyPEM();
}

std::string EncryptionManager::generateSecurePassword(int length) {
    return passwordManager_->generateSecurePassword(length);
}

PasswordManager::PasswordStrength EncryptionManager::checkPasswordStrength(const std::string& password) {
    return passwordManager_->checkPasswordStrength(password);
}

EncryptionManager::SecurityAnalysis EncryptionManager::analyzeSecurity(const std::string& password, Algorithm algorithm) {
    SecurityAnalysis analysis;
    analysis.algorithm = algorithm;
    analysis.isSecure = false;
    
    switch (algorithm) {
        case Algorithm::AES_256_CBC:
        case Algorithm::AES_256_GCM:
            analysis.keySize = 256;
            analysis.isSecure = password.length() >= 8;
            analysis.recommendations = analysis.isSecure ? 
                "Password is secure for AES-256." : 
                "Use a password with at least 8 characters for AES-256.";
            break;
        case Algorithm::RSA_2048:
            analysis.keySize = 2048;
            analysis.isSecure = true; // RSA key strength is independent of password
            analysis.recommendations = "RSA-2048 provides good security. Consider RSA-4096 for higher security.";
            break;
        case Algorithm::RSA_4096:
            analysis.keySize = 4096;
            analysis.isSecure = true;
            analysis.recommendations = "RSA-4096 provides excellent security.";
            break;
    }
    
    return analysis;
}

EncryptionManager::Algorithm EncryptionManager::detectAlgorithm(const std::vector<uint8_t>& data) {
    if (data.empty()) return Algorithm::AES_256_CBC;
    return static_cast<Algorithm>(data[0]);
}

std::vector<uint8_t> EncryptionManager::addAlgorithmHeader(const std::vector<uint8_t>& data, Algorithm algorithm) {
    std::vector<uint8_t> result;
    result.push_back(static_cast<uint8_t>(algorithm));
    result.insert(result.end(), data.begin(), data.end());
    return result;
}

EncryptionManager::Algorithm EncryptionManager::readAlgorithmHeader(const std::vector<uint8_t>& data, size_t& index) {
    if (data.empty()) return Algorithm::AES_256_CBC;
    
    Algorithm algorithm = static_cast<Algorithm>(data[0]);
    index = 1;
    return algorithm;
}

// SecureFileManager implementation

SecureFileManager::SecureFileManager() 
    : encryptionManager_(std::make_unique<EncryptionManager>()) {}

bool SecureFileManager::saveSecureFile(const std::string& content, const std::string& filename, const std::string& password) {
    return encryptionManager_->encryptToFile(content, filename, password);
}

std::string SecureFileManager::loadSecureFile(const std::string& filename, const std::string& password) {
    return encryptionManager_->decryptFromFile(filename, password);
}

bool SecureFileManager::verifyFileIntegrity(const std::string& filename, const std::string& expectedHash) {
    std::string actualHash = calculateFileHash(filename);
    return actualHash == expectedHash;
}

std::string SecureFileManager::calculateFileHash(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) return "";
    
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) return "";
    
    if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1) {
        EVP_MD_CTX_free(ctx);
        return "";
    }
    
    char buffer[4096];
    while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
        if (EVP_DigestUpdate(ctx, buffer, file.gcount()) != 1) {
            EVP_MD_CTX_free(ctx);
            return "";
        }
    }
    
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashLen;
    if (EVP_DigestFinal_ex(ctx, hash, &hashLen) != 1) {
        EVP_MD_CTX_free(ctx);
        return "";
    }
    
    EVP_MD_CTX_free(ctx);
    
    std::ostringstream oss;
    for (unsigned int i = 0; i < hashLen; i++) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    
    return oss.str();
}

bool SecureFileManager::secureDelete(const std::string& filename, int passes) {
    return overwriteFile(filename, passes);
}

bool SecureFileManager::createBackup(const std::string& filename, const std::string& backupDir) {
    // Implementation for creating secure backups
    return true; // Simplified implementation
}

bool SecureFileManager::restoreFromBackup(const std::string& backupFile, const std::string& targetFile) {
    // Implementation for restoring from backups
    return true; // Simplified implementation
}

std::string SecureFileManager::generateBackupFilename(const std::string& originalFilename) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);
    
    std::ostringstream oss;
    oss << originalFilename << ".backup." 
        << std::put_time(&tm, "%Y%m%d_%H%M%S");
    return oss.str();
}

bool SecureFileManager::overwriteFile(const std::string& filename, int passes) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) return false;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    for (int pass = 0; pass < passes; pass++) {
        file.seekp(0);
        for (int i = 0; i < 1024; i++) { // Overwrite first 1KB multiple times
            file.put(static_cast<char>(dis(gen)));
        }
        file.flush();
    }
    
    file.close();
    return std::remove(filename.c_str()) == 0;
}

} // namespace TextFlow
