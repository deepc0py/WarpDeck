#pragma once

#include <string>
#include <map>
#include <memory>

namespace warpdeck {

struct TrustedPeer {
    std::string device_id;
    std::string fingerprint;
    std::string name; // cached for display purposes
};

class SecurityManager {
public:
    SecurityManager();
    ~SecurityManager();

    bool initialize(const std::string& config_dir);
    
    // Certificate management
    bool generate_certificate_if_needed();
    std::string get_certificate_fingerprint() const;
    std::string get_certificate_file_path() const;
    std::string get_private_key_file_path() const;
    
    // Trust management
    bool is_peer_trusted(const std::string& device_id, const std::string& fingerprint) const;
    void add_trusted_peer(const std::string& device_id, const std::string& fingerprint, const std::string& name);
    void remove_trusted_peer(const std::string& device_id);
    std::map<std::string, TrustedPeer> get_trusted_peers() const;
    
    // Certificate validation
    bool validate_certificate_fingerprint(const std::string& cert_pem, const std::string& expected_fingerprint) const;
    std::string calculate_fingerprint_from_pem(const std::string& cert_pem) const;

private:
    bool load_trust_store();
    bool save_trust_store();
    std::string calculate_sha256_fingerprint(const std::string& data) const;
    
    std::string config_dir_;
    std::string trust_store_path_;
    std::string cert_file_path_;
    std::string key_file_path_;
    
    std::map<std::string, TrustedPeer> trusted_peers_;
    std::string certificate_fingerprint_;
};

} // namespace warpdeck