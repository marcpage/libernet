#ifndef __OpenSLLHelpers_h__
#define __OpenSLLHelpers_h__

#include "os/Exception.h"
#include <string.h> // strlen

#if OpenSSLAvailable
#include <openssl/err.h>
#include <openssl/evp.h>
#endif

#if __APPLE_CC__ || __APPLE__
#include <CommonCrypto/CommonCryptor.h>
#endif

namespace crypto {

class Exception : public msg::Exception {
public:
  /// Generic crypto exception
  explicit Exception(const std::string &message, const char *file = NULL,
                     int line = 0) throw()
      : msg::Exception(message, file, line) {}
  /// destructs _message
  // cppcheck-suppress missingOverride
  virtual ~Exception() throw() {}
};

#define __crypto_EncryptAssert(name, condition)                                \
  if (!(condition)) {                                                          \
    throw name##Error(#condition, __FILE__, __LINE__);                         \
  } else                                                                       \
    msg::noop()
#define DeclareError(name, message)                                            \
  class name##Error : public Exception {                                       \
  public:                                                                      \
    explicit name##Error(const std::string &call, const char *file = NULL,     \
                         int line = 0) throw()                                 \
        : Exception(call + " : " message, file, line) {}                       \
    virtual ~name##Error() throw() {}                                          \
  }

DeclareError(KeySize, "Key data is incorrect size");
DeclareError(Param, "Illegal parameter value"); // not tested
DeclareError(BufferTooSmall,
             "Insufficient buffer provided for specified operation");
DeclareError(Memory, "Memory allocation failure"); // not tested
DeclareError(Alignment, "Input size was not aligned properly");
DeclareError(Decode,
             "Input data did not decode or decrypt properly"); // not tested
DeclareError(
    Unimplemented,
    "Function not implemented for the current algorithm"); // not tested
DeclareError(IVWrongSize, "Initialization Vector is the wrong size");

#undef DeclareError

#if defined(__APPLE__)

#define __crypto_CCHandle(call)                                                \
  handleCCCryptorStatus((call), #call, __FILE__, __LINE__)

void handleCCCryptorStatus(CCCryptorStatus status, const std::string &call,
                           const char *file, int line) {
  switch (status) {
  case kCCSuccess:
    break;
  case kCCParamError:
    throw ParamError(call, file, line); // not tested
  case kCCBufferTooSmall:
    throw BufferTooSmallError(call, file, line);
  case kCCMemoryFailure:
    throw MemoryError(call, file, line); // not tested
  case kCCAlignmentError:
    throw AlignmentError(call, file, line);
  case kCCDecodeError:
    throw DecodeError(call, file, line); // not tested
  case kCCUnimplemented:
    throw UnimplementedError(call, file, line); // not tested
  default:
    throw new Exception(call, file, line); // not tested
  }
}

CFStringRef CFStringCreate(const std::string &str) {
	return CFStringCreateWithBytes(kCFAllocatorDefault, reinterpret_cast<UInt8*>(str.data()), CFIndex(str.size()), kCFStringEncodingUTF8, false);
}

std::string &CFStringGet(CFStringRef str, std::string &buffer) {
	const CFIndex length = str ? CFStringGetLength(str) : 0;
	const std::string::size_t maxBytesPerUTF8Char = 4;
	CFIndex usedBufLen = 0;

	buffer.assign(maxBytesPerUTF8Char * length, '\0');

	CFIndex convertedCharacters = str ? CFStringGetBytes(str, CFRangeMake(0,length), kCFStringEncodingUTF8, UInt8('?'), false, reinterpret_cast<UInt8*>(const_cast<char*>(buffer.data())), buffer.size(), &usedBufLen) : 0;

	buffer.erase(usedBufLen);
	return buffer;
}

CFDataRef CFDataCreate(const std::string &data) {
	return CFDataCreate(kCFAllocatorDefault, reinterpret_cast<const UInt8 *>(data.data()), data.size());
}

std::string CFDataGetBytes(CFDataRef data) {
	CFIndex size = data ? CFDataGetLength(data) : 0;

	std::string value(std::string::size_type(size), '\0');
	if (value) {
		CFDataGetBytes(data, CFRangeMake(0,size), reinterpret_cast<UInt8*>(const_cast<char*>(value.data())));
	}
	return value;
}

void CFReleaseSafe(CFTypeRef &cf) {
	if (cf) {
		::CFRelease(cf);
		cf = nullptr;
	}
}

CFTypeRef CFRetainSafe(CFTypeRef &cf) {
	if (cf) {
		::CFRetain(cf);
	}
	return cf;
}

class CFError : public Exception {
public:
  explicit Exception(CFErrorRef err, const std::string &message, const char *file = NULL,
                     int line = 0) throw()
      : msg::Exception(_errorMessage(err, message), file, line) {}
  virtual ~Exception() throw() {}
private:
  static std::string _errorMessage(CFErrorRef err, const std::string &message) {
  	std::string fullMessage = message;

  	if (err) {
  		CFErrorDomain domain = CFErrorGetDomain(err);
  		CFStringRef description = CFErrorCopyDescription(err);
  		CFStringRef reason = CFErrorCopyFailureReason(err);
  		CFStringRef recovery = CFErrorCopyRecoverySuggestion(err);

  		fullMessage += CFStringGet(domain) + ":" + CFStringGet(description) + ":" + CFStringGet(reason) + ":" + recovery;

  		CFReleaseSafe(domain);
  		CFReleaseSafe(description);
  		CFReleaseSafe(reason);
  		CFReleaseSafe(recovery);
  		CFReleaseSafe(err);
  	}

  	return fullMessage;
  }
};

#define AssertNoCFError(message, err) \
  if (err) {                                                          \
    throw crypto::CFError(err, message, __FILE__, __LINE__);	  \
  } else                                                                       \
    msg::noop()

std::string serializeRSAKey(const void *key, const char *type, std::string &value) {
	OSStatus res;
	CFDataRef exported;

	if (!key) {
		printf("key is null\n");
		return "";
	}
	res = ::SecItemExport(key, kSecFormatUnknown, kSecItemPemArmour, nullptr, &exported);
	if (errSecPassphraseRequired == res) {
		CFReleaseSafe(exported);
		exported = ::SecKeyCopyExternalRepresentation(key, NULL);
		std::string buffer = data ? CFDataGetBytes(data) : std::string();

		value = std::string("-----BEGIN RSA ") + type + " KEY-----\n"
				+ text::base64Encode(buffer, text::Base64, text::SplitBase64ForPEM)
				+ "\n-----END RSA " + type + " KEY-----\n";
	} else if (!exported) {
		return "";
	} else {
		value = CFDataGetBytes(exported);
	}
	CFReleaseSafe(exported);
	return value;
}

SecKeyRef deserializeRSAKey(const std::string &serialized, SecExternalItemType itemType) {
	CFDataRef data = CFDataCreate(serialized);
    SecExternalFormat format = kSecFormatOpenSSL;
    SecItemImportExportKeyParameters params;
        CFArrayRef keyList = nullptr;
        OSStatus res;

        memset(&params, 0, sizeof(params));
        params.flags = kSecKeyNoAccessControl;
        params.passphrase = nullptr;

	res = SecItemImport(data, nullptr, &format, &itemType, kSecItemPemArmour, &params, nullptr, &keyList);

	CFIndex count = CFArrayGetCount(keyList);
	_key = CFRetainSafe(reinterpret_cast<SecKeyRef>(const_cast<void*>(CFArrayGetValueAtIndex(keyList, 0))));
	if (CFGetTypeID(_key) != SecKeyGetTypeID()) {
		throw new Exception("Wasn't a key", __FILE__, __LINE__);
	}
	CFReleaseSafe(data);
	CFReleaseSafe(keyList);
	return _key;
}

#endif // defined(__APPLE__)

#if OpenSSLAvailable

#define __crypto_OSSLHandle(call)                                              \
  handleOpenSSLResult((call), #call, __FILE__, __LINE__)

int handleOpenSSLResult(int status, const std::string &call, const char *file,
                        int line) {
  if (!status) {
    std::string buffer(512, '\0'); // tested in libernet tests
    ERR_error_string(ERR_get_error(),
                     const_cast<char *>(buffer.data())); // libernet tests
    buffer.erase(strlen(buffer.c_str()));                // libernet tests
    throw Exception(std::string("OpenSSL Error (" + call + "): ") + buffer,
                    file, line); // libernet tests
  }
  return status;
}

template <typename T>
T *handleOpenSSLResult(T *pointer, const std::string &call, const char *file,
                       int line) {
  handleOpenSSLResult(NULL == pointer ? 0 : 1, call, file, line);
  return pointer;
}

class OpenSSLContext {
public:
  OpenSSLContext() : _context(__crypto_OSSLHandle(EVP_CIPHER_CTX_new())) {}
  ~OpenSSLContext() { __crypto_OSSLHandle(EVP_CIPHER_CTX_cleanup(_context)); }
  operator const EVP_CIPHER_CTX *() const { return _context; }
  operator EVP_CIPHER_CTX *() const { return _context; }

private:
  EVP_CIPHER_CTX *_context;
  OpenSSLContext(const OpenSSLContext &);            ///< Prevent usage
  OpenSSLContext &operator=(const OpenSSLContext &); ///< Prevent usage
};

#endif //  OpenSSLAvailable

} // namespace crypto

#endif // __OpenSLLHelpers_h__
