#ifndef __AsymmetricEncrypt_h__
#define __AsymmetricEncrypt_h__

/** @file AsymmetricEncrypt.h
        @todo document
        @todo genericize like Hash to move Mac specific code into platform
   isolcation
*/

#include "os/CryptoHelpers.h"

#if OpenSSLAvailable
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#endif

namespace crypto {

class AsymmetricPublicKey {
public:
  AsymmetricPublicKey() {}
  virtual ~AsymmetricPublicKey() {} // tested on Darwin and libernet
  virtual std::string &serialize(std::string &buffer) const = 0;
  virtual bool verify(const std::string &text,
                      const std::string &signature) = 0;
  virtual std::string &encrypt(const std::string &source,
                               std::string &encrypted) = 0;

private:
  AsymmetricPublicKey(const AsymmetricPublicKey &);
  AsymmetricPublicKey &operator=(const AsymmetricPublicKey &);
};

class AsymmetricPrivateKey {
public:
  AsymmetricPrivateKey() {} // tested on Darwin and libernet
  virtual ~AsymmetricPrivateKey() {}
  virtual AsymmetricPublicKey *publicKey() = 0;
  virtual std::string &serialize(std::string &buffer) const = 0;
  virtual std::string &decrypt(const std::string &source,
                               std::string &decrypted) = 0;
  virtual std::string &sign(const std::string &text,
                            std::string &signature) = 0;

private:
  AsymmetricPrivateKey(const AsymmetricPrivateKey &);
  AsymmetricPrivateKey &operator=(const AsymmetricPrivateKey &);
};

template <typename T> class AutoClean {
public:
  AutoClean(T *p = nullptr) : data(p) {}
  ~AutoClean() { dispose(); }
  operator const T *() const { return data; }
  operator T *() const { return data; }
  T *operator->() const { return data; }
  /* move semantics with operator= may have unintended side effects
  AutoClean<T> &operator=(const AutoClean<T> &other) {
    dispose();
    data = other.data;
    other.data = nullptr;
    return *this;
  }
  */
  void dispose() {
    if (nullptr != data) {
      delete data;
      data = nullptr;
    }
  }
  T *data;

private:
  AutoClean<T> &operator=(const AutoClean<T> &); ///< Prevent usage
  AutoClean<T>(const AutoClean<T> &);            ///< Prevent usage
};

template <> inline void AutoClean<RSA>::dispose() { RSA_free(data); }
template <> inline void AutoClean<BIGNUM>::dispose() { BN_free(data); }
template <> inline void AutoClean<BIO>::dispose() { BIO_free_all(data); }
template <> inline void AutoClean<EVP_MD_CTX>::dispose() {
  EVP_MD_CTX_destroy(data);
}
template <> inline void AutoClean<EVP_PKEY>::dispose() { EVP_PKEY_free(data); }

#if OpenSSLAvailable

class OpenSSLRSA {
public:
  OpenSSLRSA() : _rsa(nullptr) {}
  OpenSSLRSA(const OpenSSLRSA &other) : _rsa(nullptr) { *this = other; }
  explicit OpenSSLRSA(const int keySize,
                      const unsigned long publicExponent = RSA_F4)
      : _rsa(nullptr) {
    init(keySize, publicExponent);
  }
  typedef RSA *(*KeyReader)(
      BIO *, RSA **, pem_password_cb *,
      void *); // PEM_read_bio_RSAPublicKey or PEM_read_bio_RSAPrivateKey
  OpenSSLRSA(const std::string &serialized, KeyReader keyReader)
      : _rsa(nullptr) {
    init(serialized, keyReader);
  }
  OpenSSLRSA &init(const std::string &serialized, KeyReader keyReader) {
    _rsa.dispose();
    AutoClean<BIO> memory(__crypto_OSSLHandle(
        BIO_new_mem_buf(serialized.data(), serialized.size())));

    _rsa.data =
        __crypto_OSSLHandle(keyReader(memory, nullptr, nullptr, nullptr));
    return *this;
  }
  OpenSSLRSA &init(const int keySize,
                   const unsigned long publicExponent = RSA_F4) {
    _rsa.dispose();
    _rsa.data = __crypto_OSSLHandle(RSA_new());
    AutoClean<BIGNUM> bigPublicExponent(__crypto_OSSLHandle(BN_new()));

    __crypto_OSSLHandle(BN_set_word(bigPublicExponent, publicExponent));
    __crypto_OSSLHandle(
        RSA_generate_key_ex(_rsa.data, keySize, bigPublicExponent, nullptr));
    return *this;
  }
  typedef int (*Cryptor)(
      int flen, const unsigned char *from, unsigned char *to, RSA *rsa,
      int padding); // RSA_public_encrypt or RSA_private_decrypt
  std::string &crypt(const std::string &source, std::string &output,
                     Cryptor cryptor, int overheadSize = 41,
                     int padding = RSA_PKCS1_OAEP_PADDING) {
    AssertMessageException(_rsa.data != nullptr);
    const int keyBytes = RSA_size(_rsa);
    int dataSize;

    __crypto_OSSLHandle(
        static_cast<long>(source.size()) <= keyBytes - overheadSize ? 1 : 0);
    output.assign(keyBytes, '\0');
    __crypto_OSSLHandle(
        (dataSize =
             cryptor(source.size(),
                     reinterpret_cast<const unsigned char *>(source.data()),
                     reinterpret_cast<unsigned char *>(
                         const_cast<char *>(output.data())),
                     _rsa, padding)) >= 0
            ? 1
            : 0);
    output.erase(dataSize);
    return output;
  }
  std::string &serializePrivate(std::string &buffer) const {
    _serializeKey(buffer, _writePrivate);
    return buffer;
  }
  std::string &serializePublic(std::string &buffer) const {
    _serializeKey(buffer, _writePublic);
    return buffer;
  }
  typedef const EVP_MD *(*MessageDigestType)(void);
  std::string &sign(const std::string &text, std::string &signature,
                    MessageDigestType messageDigestType) { // EVP_sha256
    AutoClean<EVP_MD_CTX> signer(__crypto_OSSLHandle(EVP_MD_CTX_create()));
    AutoClean<EVP_PKEY> key(__crypto_OSSLHandle(EVP_PKEY_new()));
    size_t signatureSize = 0;

    AssertMessageException(_rsa.data != nullptr);
    __crypto_OSSLHandle(EVP_PKEY_set1_RSA(key, _rsa));
    __crypto_OSSLHandle(
        EVP_DigestSignInit(signer, nullptr, messageDigestType(), nullptr, key));
    __crypto_OSSLHandle(EVP_DigestSignUpdate(signer, text.data(), text.size()));
    __crypto_OSSLHandle(EVP_DigestSignFinal(signer, nullptr, &signatureSize));
    signature.assign(signatureSize, '\0');
    __crypto_OSSLHandle(EVP_DigestSignFinal(
        signer,
        reinterpret_cast<unsigned char *>(const_cast<char *>(signature.data())),
        &signatureSize));
    return signature; // binary signature
  }
  bool verify(const std::string &text, const std::string &signature,
              MessageDigestType messageDigestType) { // EVP_sha256
    AutoClean<EVP_MD_CTX> verifier(__crypto_OSSLHandle(EVP_MD_CTX_create()));
    AutoClean<EVP_PKEY> key(__crypto_OSSLHandle(EVP_PKEY_new()));
    int status = -1;

    AssertMessageException(_rsa.data != nullptr);
    __crypto_OSSLHandle(EVP_PKEY_set1_RSA(key, _rsa));
    __crypto_OSSLHandle(EVP_DigestVerifyInit(
        verifier, nullptr, messageDigestType(), nullptr, key));
    __crypto_OSSLHandle(
        EVP_DigestVerifyUpdate(verifier, text.data(), text.size()));
    status = EVP_DigestVerifyFinal(
        verifier, reinterpret_cast<const unsigned char *>(signature.data()),
        signature.size());
    if (status == 1) {
      return true;
    }
    if (status != 0) {
      handleOpenSSLResult(0, "EVP_DigestVerifyFinal", __FILE__, // not tested
                          __LINE__);
    }
    return false;
  }
  ~OpenSSLRSA() {}

private:
  AutoClean<RSA> _rsa;
  typedef int (*KeyTypeSerializer)(BIO *b, RSA *r);
  static int _writePrivate(BIO *b, RSA *r) {
    return PEM_write_bio_RSAPrivateKey(b, r, nullptr, nullptr, 0, nullptr,
                                       nullptr);
  }
  static int _writePublic(BIO *b, RSA *r) {
    return PEM_write_bio_RSAPublicKey(b, r);
  }
  void _serializeKey(std::string &buffer,
                     KeyTypeSerializer keytypeSerializer) const {
    AutoClean<BIO> memory(__crypto_OSSLHandle(BIO_new(BIO_s_mem())));

    AssertMessageException(_rsa.data != nullptr);
    __crypto_OSSLHandle(keytypeSerializer(memory, _rsa));

    const int dataSize = BIO_pending(memory);

    buffer.assign(dataSize, '\0');
    __crypto_OSSLHandle(
        BIO_read(memory, const_cast<char *>(buffer.data()), dataSize));
  }
  OpenSSLRSA &operator=(const OpenSSLRSA &other);
};

class OpenSSLRSAAES256PublicKey : public AsymmetricPublicKey {
public:
  OpenSSLRSAAES256PublicKey() : AsymmetricPublicKey(), _rsa() {}
  OpenSSLRSAAES256PublicKey(const OpenSSLRSAAES256PublicKey &other)
      : AsymmetricPublicKey(), _rsa() {
    *this = other;
  }
  OpenSSLRSAAES256PublicKey &operator=(const OpenSSLRSAAES256PublicKey &other) {
    std::string buffer;

    _rsa.init(other._rsa.serializePublic(buffer), PEM_read_bio_RSAPublicKey);
    return *this;
  }
  explicit OpenSSLRSAAES256PublicKey(const std::string &serialized)
      : AsymmetricPublicKey(), _rsa(serialized, PEM_read_bio_RSAPublicKey) {}
  virtual ~OpenSSLRSAAES256PublicKey() {}
  std::string serialize() const {
    std::string buffer;

    return serialize(buffer);
  }
  std::string &serialize(std::string &buffer) const override {
    return _rsa.serializePublic(buffer);
  }
  bool verify(const std::string &text, const std::string &signature) override {
    return _rsa.verify(text, signature, EVP_sha256);
  }
  std::string &encrypt(const std::string &source,
                       std::string &encrypted) override {
    return _rsa.crypt(source, encrypted, RSA_public_encrypt, 41,
                      RSA_PKCS1_OAEP_PADDING);
  }

private:
  OpenSSLRSA _rsa;
};

class OpenSSLRSAAES256PrivateKey : public AsymmetricPrivateKey {
public:
  OpenSSLRSAAES256PrivateKey() : _rsa() {}
  explicit OpenSSLRSAAES256PrivateKey(const int keySize,
                                      const unsigned long publicExponent = 3)
      : _rsa(keySize, publicExponent) {}
  explicit OpenSSLRSAAES256PrivateKey(const std::string &serialized)
      : _rsa(serialized, PEM_read_bio_RSAPrivateKey) {}
  OpenSSLRSAAES256PrivateKey(const OpenSSLRSAAES256PrivateKey &other)
      : AsymmetricPrivateKey(), _rsa() {
    *this = other;
  }
  OpenSSLRSAAES256PrivateKey &
  operator=(const OpenSSLRSAAES256PrivateKey &other) {
    std::string buffer;

    _rsa.init(other._rsa.serializePrivate(buffer), PEM_read_bio_RSAPrivateKey);
    return *this;
  }
  virtual ~OpenSSLRSAAES256PrivateKey() {} // tested on Darwin and libernet
  std::string serialize() const {
    std::string buffer;

    return serialize(buffer);
  }
  std::string &serialize(std::string &buffer) const override {
    return _rsa.serializePrivate(buffer);
  }
  std::string &sign(const std::string &text, std::string &signature) override {
    return _rsa.sign(text, signature, EVP_sha256);
  }
  std::string &decrypt(const std::string &source,
                       std::string &decrypted) override {
    return _rsa.crypt(source, decrypted, RSA_private_decrypt, 0,
                      RSA_PKCS1_OAEP_PADDING);
  }
  OpenSSLRSAAES256PublicKey getPublicKey() {
    std::string buffer;

    return OpenSSLRSAAES256PublicKey(_rsa.serializePublic(buffer));
  }
  AsymmetricPublicKey *publicKey() override {
    std::string buffer;

    return new OpenSSLRSAAES256PublicKey(_rsa.serializePublic(buffer));
  }

private:
  OpenSSLRSA _rsa;
};

typedef OpenSSLRSAAES256PublicKey RSAAES256PublicKey;
typedef OpenSSLRSAAES256PrivateKey RSAAES256PrivateKey;

#endif // OpenSSLAvailable

} // namespace crypto

#endif // __AsymmetricEncrypt_h__

/*
// clang++ 2clean/AsymetricEncrypt.cpp -framework Security -framework
CoreFoundation -DOpenSSLAvailable=1 -I.. -I2Clean
-I/usr/local/Cellar/openssl@1.1/1.1.1d/include
-L/usr/local/Cellar/openssl@1.1/1.1.1d/lib -lcrypto -o /tmp/test

#include <stdio.h>
#include <string>


/*
        Asymmetric Encrypt Behaviors
                Public Key
                        1. Verify Signature
                        2. Serialize
                        3. Deserialize
                Private Key
                        1. Sign
                        2. Serialize
                        3. Deserialize
                        4. Get public key
                        5. Generate
•/


// Recommended public exponents: 3, 5, 17, 257 or 65537

#define AppleAPI 1

#if AppleAPI

#if __APPLE_CC__ || __APPLE__
        #include <CommonCrypto/CommonCryptor.h>
        #include <Security/Security.h>
#endif

std::string dumpKey(const void *key) {
        OSStatus res;
        CFDataRef exported;

        if (!key) {
                printf("key is null\n");
                return "";
        }
        res = SecItemExport(key, kSecFormatUnknown, kSecItemPemArmour, nullptr,
&exported); if (errSecPassphraseRequired == res) { printf("Passphrase
Required\n"); return ""; } else if (!exported) { printf("Unable to export key
data\n"); return "";
        }
        CFIndex size = CFDataGetLength(exported);

        std::string value(std::string::size_type(size), '\0');
        CFDataGetBytes(exported, CFRangeMake(0,size),
reinterpret_cast<UInt8*>(const_cast<char*>(value.data()))); printf("value =
'%s'\n", value.c_str()); return value;
}

int main(int argc, char* argv[]) {
        SecKeyRef publicKey, privateKey;
        OSStatus res;
        int keySize = (argc == 2) ? atoi(argv[1]) : 1024; // 1024 - 4096
increment 8

        /* Time to generate key
                1024 = 0.25 seconds
                2048 - 2 seconds
                3072 - 8 seconds
                4096 - 30 seconds
        •/

    SecItemImportExportKeyParameters params;
    SecExternalItemType itemType = kSecItemTypePublicKey;
    SecExternalFormat format = kSecFormatOpenSSL;

        memset(&params, 0, sizeof(params));
        params.flags = kSecKeyNoAccessControl;
        params.passphrase = nullptr;

        CFMutableDictionaryRef attrDict = CFDictionaryCreateMutable(nullptr, 3,
&kCFTypeDictionaryKeyCallBacks, nullptr); CFDictionaryAddValue(attrDict,
kSecAttrKeyType, kSecAttrKeyTypeRSA); CFDictionaryAddValue(attrDict,
kSecAttrKeySizeInBits, CFNumberCreate(nullptr, kCFNumberIntType, &keySize));

        res = SecKeyGeneratePair(attrDict, &publicKey, &privateKey);

        const void *elements[] = {publicKey, privateKey};
        CFArrayRef keys = CFArrayCreate(nullptr, elements, 2,
&kCFTypeArrayCallBacks); std::string keyText = dumpKey(keys); std::string pub =
dumpKey(publicKey); std::string priv = dumpKey(privateKey);

        CFArrayRef keyList = nullptr;
        CFDataRef data;

        data = CFDataCreate(nullptr, reinterpret_cast<const UInt8
*>(pub.data()), pub.size()); itemType = kSecItemTypePublicKey; res =
SecItemImport(data, nullptr, &format, &itemType, 0, &params, nullptr, &keyList);
// crash printf("res=%d\n", res); printf("length=%ld\n",
(long)CFArrayGetCount(keyList));

        SecKeyRef pubCopy =
reinterpret_cast<SecKeyRef>(const_cast<void*>(CFArrayGetValueAtIndex(keyList,
0))); printf("pubCopy = %p\n", pubCopy); dumpKey(pubCopy);

        data = CFDataCreate(nullptr, reinterpret_cast<const UInt8
*>(priv.data()), priv.size()); itemType = kSecItemTypePrivateKey; res =
SecItemImport(data, nullptr, &format, &itemType, 0, &params, nullptr, &keyList);
        printf("res=%d\n", res);
        printf("length=%ld\n", (long)CFArrayGetCount(keyList));
        printf("Keysize = %d\n", keySize);
        // SecKeyCreateEncryptedData
https://developer.apple.com/documentation/security/1643957-seckeycreateencrypteddata?language=objc

        SecKeyRef privCopy =
reinterpret_cast<SecKeyRef>(const_cast<void*>(CFArrayGetValueAtIndex(keyList,
0))); if (CFGetTypeID(privCopy) != SecKeyGetTypeID()) { printf("Not a
SecKey\n");
        }
        printf("privCopy = %p\n", privCopy);
        dumpKey(privCopy);

        return 0;
}
*/
