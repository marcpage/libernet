#ifndef __CFHelpers_h__
#define __CFHelpers_h__
#if defined(__APPLE__)

#include <CoreFoundation/CoreFoundation.h>

namespace cf {

template <typename T> inline void release(T &v) {
  if (v) {
    ::CFRelease(v);
    v = nullptr;
  }
}

template <typename T> inline T retain(T v) {
  if (v) {
    ::CFRetain(v);
  }
  return v;
}

CFStringRef stringCreate(const std::string &str, CFStringEncoding encoding) {
  return CFStringCreateWithBytes(kCFAllocatorDefault,
                                 reinterpret_cast<const UInt8 *>(str.data()),
                                 CFIndex(str.size()), encoding, false);
}

std::string &stringGet(CFStringRef str, std::string &buffer) {
  const CFIndex length = str ? CFStringGetLength(str) : 0;
  const std::string::size_type maxBytesPerUTF8Char = 4;
  CFIndex usedBufLen = 0;

  buffer.assign(maxBytesPerUTF8Char * length, '\0');

  CFIndex convertedCharacters =
      str ? CFStringGetBytes(
                str, CFRangeMake(0, length), kCFStringEncodingUTF8, UInt8('?'),
                false,
                reinterpret_cast<UInt8 *>(const_cast<char *>(buffer.data())),
                buffer.size(), &usedBufLen)
          : 0;

  void *_unused_[] = {&_unused_, &convertedCharacters};
  buffer.erase(usedBufLen);
  return buffer;
}

CFDataRef dataCreate(const std::string &data) {
  return CFDataCreate(kCFAllocatorDefault,
                      reinterpret_cast<const UInt8 *>(data.data()),
                      data.size());
}

std::string dataGetBytes(CFDataRef data) {
  CFIndex size = data ? CFDataGetLength(data) : 0;

  std::string value(std::string::size_type(size), '\0');
  if (size) {
    CFDataGetBytes(data, CFRangeMake(0, size),
                   reinterpret_cast<UInt8 *>(const_cast<char *>(value.data())));
  }
  return value;
}

enum How { get, copyOrCreate };

template <typename T> class Safe {
public:
  T value;
  Safe() : value(nullptr) {}
  Safe(T v, How how) : value(v) {
    if (get == how) {
      retain();
    }
  }
  const Safe &retain() const {
    cf::retain(value);
    return *this;
  }
  void release() { cf::release(value); }
  ~Safe() { cf::release(value); }
  operator T() const { return value; }
  CFTypeID type() const { return CFGetTypeID(value); }
  Safe &operator=(const Safe<T> &v) {
    release();
    value = v;
    retain();
  }
};

class string : public Safe<CFStringRef> {
public:
  string() : Safe<CFStringRef>() {}
  string(CFStringRef s, How how) : Safe<CFStringRef>(s, how) {}
  string(const std::string &s,
         CFStringEncoding encoding = kCFStringEncodingUTF8)
      : Safe<CFStringRef>(stringCreate(s, encoding), copyOrCreate) {}
  std::string &get(std::string &buffer) const {
    if (value) {
      stringGet(this->value, buffer);
    } else {
      buffer.clear();
    }
    return buffer;
  }
  std::string get() const {
    std::string buffer;

    return get(buffer);
  }
  operator std::string() const { return get(); }
};

class data : public Safe<CFDataRef> {
public:
  data() : Safe<CFDataRef>() {}
  data(CFDataRef d, How how) : Safe<CFDataRef>(d, how) {}
  data(const std::string &d) : Safe<CFDataRef>(dataCreate(d), copyOrCreate) {}
  std::string &get(std::string &buffer) const {
    return buffer = dataGetBytes(this->value);
  }
  std::string get() const {
    std::string buffer;

    return get(buffer);
  }
  operator std::string() const { return get(); }
};

class array : public Safe<CFArrayRef> {
public:
  array() : Safe<CFArrayRef>() {}
  array(CFArrayRef d, How how) : Safe<CFArrayRef>(d, how) {}
  template <typename T> T get(CFIndex index) {
    return reinterpret_cast<T>(
        const_cast<void *>(CFArrayGetValueAtIndex(value, index)));
  }
  CFIndex count() const { return CFArrayGetCount(value); }
};

class mutableDictionary : public Safe<CFMutableDictionaryRef> {
public:
  mutableDictionary() : Safe<CFMutableDictionaryRef>() {}
  mutableDictionary(CFIndex capacity)
      : Safe<CFMutableDictionaryRef>(
            ::CFDictionaryCreateMutable(kCFAllocatorDefault, capacity,
                                        &kCFTypeDictionaryKeyCallBacks,
                                        &kCFTypeDictionaryValueCallBacks),
            copyOrCreate) {}
  mutableDictionary(CFMutableDictionaryRef d, How how)
      : Safe<CFMutableDictionaryRef>(d, how) {}
  mutableDictionary &put(const void *key, const void *keysValue) {
    ::CFDictionaryAddValue(value, key, keysValue);
    return *this;
  }
  mutableDictionary &put(const void *key, int keysValue) {
    return put(key, ::CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType,
                                     &keysValue));
  }
};

class Error : public msg::Exception {
public:
  explicit Error(CFErrorRef err, const std::string &message,
                 const char *file = NULL, int line = 0) throw()
      : msg::Exception(_errorMessage(err, message), file, line) {}
  virtual ~Error() throw() {}

private:
  static std::string _errorMessage(CFErrorRef err, const std::string &message) {
    std::string fullMessage = message;

    if (err) {
      cf::string domain(CFErrorGetDomain(err), cf::get);
      cf::string description(CFErrorCopyDescription(err), cf::copyOrCreate);
      cf::string reason(CFErrorCopyFailureReason(err), cf::copyOrCreate);
      cf::string recovery(CFErrorCopyRecoverySuggestion(err), cf::copyOrCreate);

      fullMessage += domain.get() + ":" + description.get() + ":" +
                     reason.get() + ":" + recovery.get();

      cf::release(err);
    }

    return fullMessage;
  }
};

#define AssertNoCFError(message, err)                                          \
  if (err) {                                                                   \
    throw cf::Error(err, message, __FILE__, __LINE__);                         \
  } else                                                                       \
    msg::noop()

} // namespace cf

#endif // defined(__APPLE__)
#endif // __CFHelpers_h__
