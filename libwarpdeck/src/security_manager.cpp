#include "security_manager.h"
#include "utils.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/pem.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <openssl/evp.h>

namespace warpdeck {

SecurityManager::SecurityManager() {}

SecurityManager::~SecurityManager() {}

bool SecurityManager::initialize(const std::string& config_dir) {
    config_dir_ = config_dir;
    trust_store_path_ = config_dir_ + "/trust_store.json";
    cert_file_path_ = config_dir_ + "/cert.pem";
    key_file_path_ = config_dir_ + "/key.pem";
    
    // Create config directory if it doesn't exist
    if (!utils::create_directory(config_dir_)) {
        return false;
    }
    
    // Load existing trust store
    load_trust_store();
    
    return true;
}

bool SecurityManager::generate_certificate_if_needed() {
    // Check if certificate already exists
    if (utils::file_exists(cert_file_path_) && utils::file_exists(key_file_path_)) {
        // Load existing certificate and calculate fingerprint
        std::ifstream cert_file(cert_file_path_);
        if (cert_file) {
            std::string cert_content((std::istreambuf_iterator<char>(cert_file)),
                                   std::istreambuf_iterator<char>());
            certificate_fingerprint_ = calculate_fingerprint_from_pem(cert_content);
            return true;
        }
    }
    
    // Generate new RSA key pair
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
    if (!ctx) {
        return false;
    }
    
    if (EVP_PKEY_keygen_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        return false;
    }
    
    if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, 2048) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        return false;
    }
    
    EVP_PKEY* pkey = nullptr;
    if (EVP_PKEY_keygen(ctx, &pkey) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        return false;
    }
    EVP_PKEY_CTX_free(ctx);
    
    // Create X509 certificate
    X509* cert = X509_new();
    if (!cert) {
        EVP_PKEY_free(pkey);
        return false;
    }
    
    // Set version
    X509_set_version(cert, 2);
    
    // Set serial number
    ASN1_INTEGER_set(X509_get_serialNumber(cert), 1);
    
    // Set validity period (1 year)
    X509_gmtime_adj(X509_get_notBefore(cert), 0);
    X509_gmtime_adj(X509_get_notAfter(cert), 365 * 24 * 3600);
    
    // Set public key
    X509_set_pubkey(cert, pkey);
    
    // Set subject and issuer (self-signed)
    X509_NAME* name = X509_get_subject_name(cert);
    X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, 
                              reinterpret_cast<const unsigned char*>("US"), -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC,
                              reinterpret_cast<const unsigned char*>("WarpDeck"), -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC,
                              reinterpret_cast<const unsigned char*>("WarpDeck Device"), -1, -1, 0);
    X509_set_issuer_name(cert, name);
    
    // Sign certificate
    if (!X509_sign(cert, pkey, EVP_sha256())) {
        X509_free(cert);
        EVP_PKEY_free(pkey);
        return false;
    }
    
    // Save private key
    FILE* key_file = fopen(key_file_path_.c_str(), "wb");
    if (!key_file) {
        X509_free(cert);
        EVP_PKEY_free(pkey);
        return false;
    }
    
    PEM_write_PrivateKey(key_file, pkey, nullptr, nullptr, 0, nullptr, nullptr);
    fclose(key_file);
    
    // Save certificate
    FILE* cert_file = fopen(cert_file_path_.c_str(), "wb");
    if (!cert_file) {
        X509_free(cert);
        EVP_PKEY_free(pkey);
        return false;
    }
    
    PEM_write_X509(cert_file, cert);
    fclose(cert_file);
    
    // Calculate fingerprint
    unsigned char* cert_der = nullptr;
    int cert_der_len = i2d_X509(cert, &cert_der);
    if (cert_der_len > 0) {
        certificate_fingerprint_ = calculate_sha256_fingerprint(
            std::string(reinterpret_cast<char*>(cert_der), cert_der_len));
        OPENSSL_free(cert_der);
    }
    
    X509_free(cert);
    EVP_PKEY_free(pkey);
    
    return !certificate_fingerprint_.empty();
}

std::string SecurityManager::get_certificate_fingerprint() const {
    return certificate_fingerprint_;
}

std::string SecurityManager::get_certificate_file_path() const {
    return cert_file_path_;
}

std::string SecurityManager::get_private_key_file_path() const {
    return key_file_path_;
}

bool SecurityManager::is_peer_trusted(const std::string& device_id, const std::string& fingerprint) const {
    auto it = trusted_peers_.find(device_id);
    if (it == trusted_peers_.end()) {
        return false;
    }
    return it->second.fingerprint == fingerprint;
}

void SecurityManager::add_trusted_peer(const std::string& device_id, const std::string& fingerprint, const std::string& name) {
    TrustedPeer peer;
    peer.device_id = device_id;
    peer.fingerprint = fingerprint;
    peer.name = name;
    
    trusted_peers_[device_id] = peer;
    save_trust_store();
}

void SecurityManager::remove_trusted_peer(const std::string& device_id) {
    trusted_peers_.erase(device_id);
    save_trust_store();
}

std::map<std::string, TrustedPeer> SecurityManager::get_trusted_peers() const {
    return trusted_peers_;
}

bool SecurityManager::validate_certificate_fingerprint(const std::string& cert_pem, const std::string& expected_fingerprint) const {
    std::string actual_fingerprint = calculate_fingerprint_from_pem(cert_pem);
    return actual_fingerprint == expected_fingerprint;
}

std::string SecurityManager::calculate_fingerprint_from_pem(const std::string& cert_pem) const {
    BIO* bio = BIO_new_mem_buf(cert_pem.c_str(), -1);
    if (!bio) {
        return "";
    }
    
    X509* cert = PEM_read_bio_X509(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    
    if (!cert) {
        return "";
    }
    
    unsigned char* cert_der = nullptr;
    int cert_der_len = i2d_X509(cert, &cert_der);
    X509_free(cert);
    
    if (cert_der_len <= 0) {
        return "";
    }
    
    std::string fingerprint = calculate_sha256_fingerprint(
        std::string(reinterpret_cast<char*>(cert_der), cert_der_len));
    OPENSSL_free(cert_der);
    
    return fingerprint;
}

bool SecurityManager::load_trust_store() {
    if (!utils::file_exists(trust_store_path_)) {
        return true; // Empty trust store is valid
    }
    
    std::ifstream file(trust_store_path_);
    if (!file) {
        return false;
    }
    
    try {
        nlohmann::json j;
        file >> j;
        
        trusted_peers_.clear();
        for (const auto& peer_json : j) {
            TrustedPeer peer;
            peer.device_id = peer_json["device_id"];
            peer.fingerprint = peer_json["fingerprint"];
            peer.name = peer_json["name"];
            
            trusted_peers_[peer.device_id] = peer;
        }
        
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool SecurityManager::save_trust_store() {
    try {
        nlohmann::json j = nlohmann::json::array();
        
        for (const auto& [device_id, peer] : trusted_peers_) {
            nlohmann::json peer_json;
            peer_json["device_id"] = peer.device_id;
            peer_json["fingerprint"] = peer.fingerprint;
            peer_json["name"] = peer.name;
            j.push_back(peer_json);
        }
        
        std::ofstream file(trust_store_path_);
        if (!file) {
            return false;
        }
        
        file << j.dump(2);
        return true;
        
    } catch (const std::exception&) {
        return false;
    }
}

std::string SecurityManager::calculate_sha256_fingerprint(const std::string& data) const {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(data.c_str()), data.length(), hash);
    
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    
    return ss.str();
}

} // namespace warpdeck