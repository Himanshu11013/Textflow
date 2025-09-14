#pragma once

#include <string>
#include <vector>
#include <memory>
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

namespace TextFlow {

// AES encryption implementation
class AESEncryption {
public:
    AESEncryption();
    ~AESEncryption();
    
    // Encryption/Decryption
    std::vector<uint8_t> encrypt(const std::string& plaintext, const std::string& password);
    std::string decrypt(const std::vector<uint8_t>& ciphertext, const std::string& password);
    
    // File operations
    bool encryptToFile(const std::string& plaintext, const std::string& filename, const std::string& password);
    std::string decryptFromFile(const std::string& filename, const std::string& password);
    
    // Key derivation
    std::vector<uint8_t> deriveKey(const std::string& password, const std::vector<uint8_t>& salt);
    
    // Utility
    bool isValidPassword(const std::string& password) const;
    std::string generateRandomSalt();
    std::string generateRandomIV();

private:
    static const int KEY_SIZE = 32; // 256 bits
    static const int IV_SIZE = 16;  // 128 bits
    static const int SALT_SIZE = 16; // 128 bits
    static const int ITERATIONS = 10000; // PBKDF2 iterations
    
    EVP_CIPHER_CTX* encryptCtx_;
    EVP_CIPHER_CTX* decryptCtx_;
    
    void initializeContexts();
    void cleanupContexts();
    std::vector<uint8_t> generateRandomBytes(int size);
};

// RSA encryption for key exchange (optional)
class RSAEncryption {
public:
    RSAEncryption();
    ~RSAEncryption();
    
    // Key generation
    bool generateKeyPair(int keySize = 2048);
    std::string getPublicKeyPEM() const;
    std::string getPrivateKeyPEM() const;
    
    // Encryption/Decryption
    std::vector<uint8_t> encrypt(const std::string& plaintext, const std::string& publicKeyPEM);
    std::string decrypt(const std::vector<uint8_t>& ciphertext, const std::string& privateKeyPEM);
    
    // Key management
    bool loadPublicKey(const std::string& publicKeyPEM);
    bool loadPrivateKey(const std::string& privateKeyPEM);
    bool saveKeyPair(const std::string& publicKeyFile, const std::string& privateKeyFile) const;

private:
    EVP_PKEY* publicKey_;
    EVP_PKEY* privateKey_;
    
    void cleanupKeys();
};

// Password hashing and verification
class PasswordManager {
public:
    PasswordManager();
    ~PasswordManager() = default;
    
    // Password hashing
    std::string hashPassword(const std::string& password);
    bool verifyPassword(const std::string& password, const std::string& hash);
    
    // Password strength checking
    struct PasswordStrength {
        int score;
        std::string feedback;
        bool isStrong;
    };
    
    PasswordStrength checkPasswordStrength(const std::string& password);
    
    // Secure password generation
    std::string generateSecurePassword(int length = 16, bool includeSymbols = true);

private:
    static const int SALT_SIZE = 32;
    static const int HASH_SIZE = 32;
    static const int ITERATIONS = 100000;
    
    std::vector<uint8_t> generateSalt();
    std::string bytesToHex(const std::vector<uint8_t>& bytes);
    std::vector<uint8_t> hexToBytes(const std::string& hex);
};

// Encryption manager
class EncryptionManager {
public:
    enum class Algorithm {
        AES_256_CBC,
        AES_256_GCM,
        RSA_2048,
        RSA_4096
    };
    
    EncryptionManager();
    ~EncryptionManager() = default;
    
    // Main encryption interface
    std::vector<uint8_t> encrypt(const std::string& plaintext, const std::string& password, 
                                Algorithm algorithm = Algorithm::AES_256_CBC);
    std::string decrypt(const std::vector<uint8_t>& ciphertext, const std::string& password,
                       Algorithm algorithm = Algorithm::AES_256_CBC);
    
    // File operations
    bool encryptToFile(const std::string& plaintext, const std::string& filename, 
                      const std::string& password, Algorithm algorithm = Algorithm::AES_256_CBC);
    std::string decryptFromFile(const std::string& filename, const std::string& password);
    
    // Key management
    bool generateKeyPair(const std::string& publicKeyFile, const std::string& privateKeyFile,
                        Algorithm algorithm = Algorithm::RSA_2048);
    std::string getPublicKey() const;
    std::string getPrivateKey() const;
    
    // Password utilities
    std::string generateSecurePassword(int length = 16);
    PasswordManager::PasswordStrength checkPasswordStrength(const std::string& password);
    
    // Security analysis
    struct SecurityAnalysis {
        Algorithm algorithm;
        int keySize;
        bool isSecure;
        std::string recommendations;
    };
    
    SecurityAnalysis analyzeSecurity(const std::string& password, Algorithm algorithm);

private:
    std::unique_ptr<AESEncryption> aes_;
    std::unique_ptr<RSAEncryption> rsa_;
    std::unique_ptr<PasswordManager> passwordManager_;
    
    Algorithm detectAlgorithm(const std::vector<uint8_t>& data);
    std::vector<uint8_t> addAlgorithmHeader(const std::vector<uint8_t>& data, Algorithm algorithm);
    Algorithm readAlgorithmHeader(const std::vector<uint8_t>& data, size_t& index);
};

// Secure file operations
class SecureFileManager {
public:
    SecureFileManager();
    ~SecureFileManager() = default;
    
    // Secure file operations
    bool saveSecureFile(const std::string& content, const std::string& filename, 
                       const std::string& password);
    std::string loadSecureFile(const std::string& filename, const std::string& password);
    
    // File integrity
    bool verifyFileIntegrity(const std::string& filename, const std::string& expectedHash);
    std::string calculateFileHash(const std::string& filename);
    
    // Secure deletion
    bool secureDelete(const std::string& filename, int passes = 3);
    
    // Backup and recovery
    bool createBackup(const std::string& filename, const std::string& backupDir);
    bool restoreFromBackup(const std::string& backupFile, const std::string& targetFile);

private:
    std::unique_ptr<EncryptionManager> encryptionManager_;
    
    std::string generateBackupFilename(const std::string& originalFilename);
    bool overwriteFile(const std::string& filename, int passes);
};

} // namespace TextFlow
