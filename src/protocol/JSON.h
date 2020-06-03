#ifndef __JSON_h__
#define __JSON_h__

/** @file JSON.h
        @todo document
*/

#include "os/Exception.h"
#include <iomanip>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string.h> // strlen
#include <string>
#include <vector>

namespace json {

enum Type {
  NullType,
  ObjectType,
  ArrayType,
  StringType,
  IntegerType,
  RealType,
  BooleanType
};

#define TypeCase(name)                                                         \
  case name:                                                                   \
    return std::string(#name).substr(0, strlen(#name) - strlen("Type"))
#define CheckType(found, expected)                                             \
  if (expected != found) {                                                     \
    throw WrongType(expected, found, __FILE__, __LINE__);                      \
  }
#define Check2Types(found, expected1, expected2)                               \
  if ((expected1 != found) && (expected2 != found)) {                          \
    throw WrongType(expected1, expected2, found, __FILE__, __LINE__);          \
  }
#define Check3Types(found, expected1, expected2, expected3)                    \
  if ((expected1 != found) && (expected2 != found) && (expected3 != found)) {  \
    throw WrongType(expected1, expected2, expected3, found, __FILE__,          \
                    __LINE__);                                                 \
  }

class WrongType : public msg::Exception {
public:
  static const std::string name(Type type) {
    switch (type) {
      TypeCase(NullType);
      TypeCase(ObjectType);
      TypeCase(ArrayType);
      TypeCase(StringType);
      TypeCase(IntegerType);
      TypeCase(RealType);
      TypeCase(BooleanType);
    default:
      break; // not tested
    }
    return "Corrupt"; // not tested
  }
  explicit WrongType(const std::string &message, const char *file = NULL,
                     int line = 0) throw()
      : msg::Exception(message, file, line) {}
  WrongType(Type expected, Type found, const char *file = NULL,
            int line = 0) throw()
      : msg::Exception(std::string("Expected ") + name(expected) + " Found " +
                           name(found),
                       file, line) {}
  WrongType(Type expected1, Type expected2, Type found, const char *file = NULL,
            int line = 0) throw()
      : msg::Exception(std::string("Expected ") + name(expected1) + " or " +
                           name(expected2) + " Found " + name(found),
                       file, line) {}
  WrongType(Type expected1, Type expected2, Type expected3, Type found,
            const char *file = NULL, int line = 0) throw()
      : msg::Exception(std::string("Expected ") + name(expected1) + " " +
                           name(expected2) + " or " + name(expected3) +
                           " Found " + name(found),
                       file, line) {}
  virtual ~WrongType() throw() {}
};

class Value {
public:
  typedef std::vector<std::string> StringList;
  Value(Type t = NullType) : _value(_create(t)) {}
  explicit Value(const std::string &text) : _value(NULL) { parse(text); }
  Value(const Value &other)
      : _value((NULL == other._value) ? reinterpret_cast<Instance *>(NULL)
                                      : other._value->clone()) {}
  ~Value() { makeNull(); }
  Type type() const { return NULL == _value ? NullType : _value->type(); }
  bool is(Type t) const { return t == type(); }
  Value &parse(const std::string &text);
  Value &parse(const std::string &text, std::string::size_type &offset);
  bool boolean() const;
  int64_t integer() const;
  double real() const;
  const std::string &string() const;
  int count() const;
  StringList keys() const;
  Value &clear();
  Value &erase(int startIndex, int endIndex = -1);
  Value &erase(const std::string &key);
  bool has(const std::string &key);
  Value &makeObject();
  Value &makeArray();
  Value &makeNull() {
    delete _value;
    _value = NULL;
    return *this;
  }
  Value &append(const Value &value);
  Value &insert(const Value &value, int beforeIndex);
  std::string &format(std::string &buffer, int indent = -1,
                      int indentLevel = 0) const;
  std::string format(int indent = -1, int indentLevel = 0) const;
  /* prevent common coding errors.
  call formet() to get json text and string() to get string contents.

  operator std::string() const {
    std::string buffer;
    return format(buffer);
  }
  */
  Value &operator=(const Value &other) {
    if (this == &other) {
      return *this; // not tested
    }
    makeNull();
    _value = (NULL == other._value) ? reinterpret_cast<Instance *>(NULL)
                                    : other._value->clone();
    return *this;
  }
  Value &operator=(int value);
  Value &operator=(int64_t value);
  Value &operator=(double value);
  Value &operator=(const std::string &value);
  Value &operator=(const char *value);
  Value &operator=(bool value);
  Value &operator+=(int64_t value) { return *this = (integer() + value); }
  Value &operator+=(int value) { return *this = (integer() + int64_t(value)); }
  Value &operator-=(int64_t value) { return *this = (integer() - value); }
  Value &operator-=(int value) { return *this = (integer() - int64_t(value)); }
  Value &operator[](int index);
  const Value &operator[](int index) const;
  Value &operator[](const std::string &key);
  const Value &operator[](const std::string &key) const;
  bool operator==(const Value &other) const;
  bool operator!=(const Value &other) const { return !(*this == other); }

private:
  class Instance {
  public:
    Instance() {}
    virtual ~Instance() {}
    virtual Type type() const = 0;
    virtual Instance *clone() const = 0;
    virtual void format(std::string &buffer, int indent,
                        int indentLevel) const = 0;
    virtual bool operator==(const Instance &other) const = 0;

  private:
    Instance(const Instance &);
    Instance &operator=(const Instance &);
  };

  class String : public Instance {
  public:
    explicit String(const std::string &value) : _value(value) {}
    virtual ~String() {}
    Type type() const override { return StringType; }
    Instance *clone() const override { return new String(_value); }
    void format(std::string &buffer, int indent,
                int indentLevel) const override;
    bool operator==(const Instance &other) const override {
      if (type() != other.type()) {
        return false; // not tested
      }
      return reinterpret_cast<const String *>(&other)->value() == value();
    }
    std::string &value() { return _value; }
    const std::string &value() const { return _value; }
    int count() const { return _value.length(); }
    void clear() { _value.clear(); }
    void erase(int startIndex, int endIndex) {
      _value.erase(startIndex,
                   (endIndex < startIndex ? _value.length() : endIndex) -
                       startIndex);
    }

  private:
    std::string _value;
    String(const String &);
    String &operator=(const String &);
  };

  class Integer : public Instance {
  public:
    explicit Integer(int64_t value) : _value(value) {}
    virtual ~Integer() {}
    Type type() const override { return IntegerType; }
    Instance *clone() const override { return new Integer(_value); }
    void format(std::string &buffer, int /*indent*/,
                int /*indentLevel*/) const override {
      buffer = std::to_string(_value);
    }
    bool operator==(const Instance &other) const override {
      if (type() != other.type()) {
        return false; // not tested
      }
      return reinterpret_cast<const Integer *>(&other)->value() == value();
    }
    int64_t &value() { return _value; }
    const int64_t &value() const { return _value; }

  private:
    int64_t _value;
    Integer(const Integer &);
    Integer &operator=(const Integer &);
  };

  class Real : public Instance {
  public:
    explicit Real(double value) : _value(value) {}
    virtual ~Real() {}
    Type type() const override { return RealType; }
    Instance *clone() const override { return new Real(_value); }
    void format(std::string &buffer, int /*indent*/,
                int /*indentLevel*/) const override {
      buffer = std::to_string(_value);
    }
    bool operator==(const Instance &other) const override {
      if (type() != other.type()) {
        return false; // not tested
      }
      return reinterpret_cast<const Real *>(&other)->value() == value();
    }
    double &value() { return _value; }
    const double &value() const { return _value; }

  private:
    double _value;
    Real(const Real &);
    Real &operator=(const Real &);
  };

  class Boolean : public Instance {
  public:
    explicit Boolean(bool value) : _value(value) {}
    virtual ~Boolean() {}
    Type type() const override { return BooleanType; }
    Instance *clone() const override { return new Boolean(_value); }
    void format(std::string &buffer, int /*indent*/,
                int /*indentLevel*/) const override {
      buffer = _value ? "true" : "false";
    }
    bool operator==(const Instance &other) const override {
      if (type() != other.type()) {
        return false; // not tested
      }
      return reinterpret_cast<const Boolean *>(&other)->value() == value();
    }
    bool &value() { return _value; }
    const bool &value() const { return _value; }

  private:
    bool _value;
    Boolean(const Boolean &);
    Boolean &operator=(const Boolean &);
  };

  class Array : public Instance {
  public:
    Array() : _value() {}
    virtual ~Array() {}
    Type type() const override { return ArrayType; }
    Instance *clone() const override;
    void format(std::string &buffer, int indent,
                int indentLevel) const override;
    bool operator==(const Instance &other) const override;
    const Value &get(int index) const { return _value[index]; }
    Value &get(int index) { return _value[index]; }
    void clear() { _value.clear(); }
    void erase(int startIndex, int endIndex) {
      _value.erase(_value.begin() + startIndex, _value.begin() + endIndex);
    }
    int count() const { return _value.size(); }
    void append(const Value &value) { _value.push_back(value); }
    /// @todo Test
    void insert(const Value &value, int beforeIndex) {
      _value.insert(_value.begin() + beforeIndex, value);
    }

  private:
    std::vector<Value> _value;
    Array(const Array &);
    Array &operator=(const Array &);
  };

  class Object : public Instance {
  public:
    Object() : _value() {}
    virtual ~Object() {}
    Type type() const override { return ObjectType; }
    Instance *clone() const override;
    void format(std::string &buffer, int indent,
                int indentLevel) const override;
    bool operator==(const Instance &other) const override;
    StringList keys() const;
    const Value &get(const std::string &key) const;
    Value &get(const std::string &key) { return _value[key]; }
    void clear() { _value.clear(); }
    void erase(const std::string &key) { _value.erase(key); }
    int count() const { return _value.size(); }
    bool has(const std::string &key) const {
      return _value.find(key) != _value.end();
    }

  private:
    std::map<std::string, Value> _value;
    Object(const Object &);
    Object &operator=(const Object &);
  };
  Instance *_value;
  std::string::size_type _skipWhitespace(const std::string &text,
                                         std::string::size_type start);
  void _parseObject(const std::string &text, std::string::size_type &offset);
  void _parseArray(const std::string &text, std::string::size_type &offset);
  void _parseString(const std::string &text, std::string::size_type &offset);
  void _parseWord(const std::string &text, const std::string &expected,
                  std::string::size_type &offset);
  void _parseNumber(const std::string &text, std::string::size_type &offset);
  static size_t _codepoint(const std::string &text,
                           std::string::size_type &offset);
  static std::string _utf8(size_t codepoint);
  static Instance *_create(Type t);
};

inline std::string::size_type
Value::_skipWhitespace(const std::string &text, std::string::size_type start) {
  while ((start < text.length()) && ::isspace(text[start])) {
    start += 1;
  }
  return start;
}
inline void Value::_parseObject(const std::string &text,
                                std::string::size_type &offset) {
  Value value;
  Value key;

  makeObject();
  offset += 1; // skip {
  while ((offset < text.length()) && (text[offset] != '}')) {
    offset = _skipWhitespace(text, offset);
    if ('}' != text[offset]) {
      if (offset >= text.length()) {
        throw WrongType(std::string("Illegal termination of object: "),
                        __FILE__, __LINE__);
      } else if ('"' != text[offset]) {
        throw WrongType(std::string("Illegal character: ") +
                            text.substr(offset, 1),
                        __FILE__, __LINE__);
      }
      key.parse(text, offset);
      offset = _skipWhitespace(text, offset);
      if (offset >= text.length()) {
        throw WrongType(std::string("Illegal termination of object: "),
                        __FILE__, __LINE__);
      } else if (':' != text[offset]) {
        throw WrongType(std::string("Illegal character (expected colon(:)): ") +
                            text.substr(offset, 1),
                        __FILE__, __LINE__);
      }
      offset += 1; // skip :
      reinterpret_cast<Object *>(_value)->get(key.string()) =
          value.parse(text, offset);
      offset = _skipWhitespace(text, offset);
      if ((',' != text[offset]) && ('}' != text[offset])) {
        throw WrongType(std::string("Illegal character: ") +
                            text.substr(offset, 1),
                        __FILE__, __LINE__);
      }
      if (',' == text[offset]) {
        offset += 1; // skip ,
        offset = _skipWhitespace(text, offset);
      }
    }
  }
  offset += 1; // skip }
  if (offset > text.length()) {
    throw WrongType("Unterminated Object", __FILE__, __LINE__);
  }
}
inline void Value::_parseArray(const std::string &text,
                               std::string::size_type &offset) {
  Value value;

  makeArray();
  offset += 1; // skip [
  while ((offset < text.length()) && (text[offset] != ']')) {
    offset = _skipWhitespace(text, offset);
    if (']' != text[offset]) {
      reinterpret_cast<Array *>(_value)->append(value.parse(text, offset));
      offset = _skipWhitespace(text, offset);
      if ((',' != text[offset]) && (']' != text[offset])) {
        throw WrongType(std::string("Illegal character: ") +
                            text.substr(offset, 1),
                        __FILE__, __LINE__);
      }
      if (',' == text[offset]) {
        offset += 1; // skip ,
        offset = _skipWhitespace(text, offset);
      }
    }
  }
  offset += 1; // skip ]
  if (offset > text.length()) {
    throw WrongType("Unterminated Array", __FILE__, __LINE__);
  }
}
inline void Value::_parseString(const std::string &text,
                                std::string::size_type &offset) {
  std::string result;
  long value;
  size_t count = 0;

  offset += 1; // skip quote
  do {
    if (offset >= text.length()) {
      throw WrongType(std::string("Unterminated string"), __FILE__, __LINE__);
    }
    switch (text[offset]) {
    case '\\':
      offset += 1;
      switch (text[offset]) {
      case '/':
        result += '/';
        break;
      case '"':
        result += '"';
        break;
      case '\\':
        result += '\\';
        break;
      case 'b':
        result += '\b';
        break;
      case 'f':
        result += '\f';
        break;
      case 'r':
        result += '\r';
        break;
      case 'n':
        result += '\n';
        break;
      case 't':
        result += '\t';
        break;
      case 'u':
        if (text[offset + 1] == '{') {
          const std::string::size_type end = text.find('}', offset + 2);
          const std::string::size_type size = end - offset - 2;

          if (std::string::npos == end) {
            throw WrongType(
                std::string("Missing } for ECMAScript 6+ \\u{xxxxxxxx}"),
                __FILE__, __LINE__);
          }
          if (offset + 2 == end) {
            throw WrongType(
                std::string("Empty codepoint for ECMAScript 6+ \\u{xxxxxxxx}"),
                __FILE__, __LINE__);
          }
          value = std::stol(text.substr(offset + 2, size), &count, 16);
          if (count != size) {
            throw WrongType(std::string("Illegal hex: ") +
                                text.substr(offset + 1, size),
                            __FILE__, __LINE__);
          }
          offset += 1 + size + 1;
        } else {
          value = std::stol(text.substr(offset + 1, 4), &count, 16);
          if (count != 4) {
            throw WrongType(std::string("Illegal hex: ") +
                                text.substr(offset + 1, 4),
                            __FILE__, __LINE__);
          }
          offset += 4;
        }
        result += _utf8(value);
        break;
      default:
        throw WrongType(std::string("Illegal escape: ") + text[offset],
                        __FILE__, __LINE__);
      }
      offset += 1;
      break;
    case '"':
      break;
    default:
      result += text[offset];
      offset += 1;
      break;
    }
  } while ('"' != text[offset]);
  offset += 1; // skip quote
  *this = result;
}
inline void Value::_parseWord(const std::string &text,
                              const std::string &expected,
                              std::string::size_type &offset) {
  if (text.substr(offset, expected.length()) != expected) {
    throw WrongType(std::string("Illegal Word: ") +
                        text.substr(offset, expected.length()),
                    __FILE__, __LINE__);
  }
  offset += expected.length();
}
inline void Value::_parseNumber(const std::string &text,
                                std::string::size_type &offset) {
  std::string integerChars("-+0123456789");
  std::string realChars("Ee.");
  bool hasRealChar = false;
  const std::string::size_type start = offset;
  size_t after = 0;

  while ((offset < text.length()) &&
         ((integerChars.find(text[offset]) != std::string::npos) ||
          ((realChars.find(text[offset]) != std::string::npos)))) {
    hasRealChar =
        hasRealChar || (realChars.find(text[offset]) != std::string::npos);
    offset += 1;
  }
  if (start == offset) {
    throw WrongType(std::string("Illegal Character: ") + text.substr(start, 1),
                    __FILE__, __LINE__);
  }
  if (hasRealChar) {
    try {
      *this = std::stod(text.substr(start, offset - start), &after);
    } catch (const std::exception &exception) {
      throw WrongType(std::string("Illegal Number: ") +
                          text.substr(start, offset - start),
                      __FILE__, __LINE__);
    }
  } else {
    try {
      *this = static_cast<int64_t>(
          std::stoll(text.substr(start, offset - start), &after));
    } catch (const std::exception &exception) {
      throw WrongType(std::string("Illegal Number: ") +
                          text.substr(start, offset - start),
                      __FILE__, __LINE__);
    }
  }
  if (after != offset - start) {
    throw WrongType(std::string("Illegal Number: ") +
                        text.substr(start, offset - start),
                    __FILE__, __LINE__);
  }
}
inline size_t Value::_codepoint(const std::string &text,
                                std::string::size_type &offset) {
  /*
          1	7	U+0000	U+007F  	0xxxxxxx
          2	11	U+0080	U+07FF  	110xxxxx	10xxxxxx
          3	16	U+0800	U+FFFF  	1110xxxx	10xxxxxx
     10xxxxxx
          4	21	U+10000	U+10FFFF	11110xxx	10xxxxxx
     10xxxxxx	10xxxxxx
  */
  const bool oneByte =
      (offset < text.length()) && ((0x80 & text.data()[offset]) == 0);
  const bool twoBytes =
      (offset + 1 < text.length()) && ((0xE0 & text.data()[offset]) == 0xC0);
  const bool threeBytes =
      (offset + 2 < text.length()) && ((0xF0 & text.data()[offset]) == 0xE0);
  const bool fourBytes =
      (offset + 3 < text.length()) && ((0xF8 & text.data()[offset]) == 0xF0);
  size_t codepoint;

  if (oneByte) {
    offset += 1;
    codepoint = text.data()[offset - 1] & 0xFF;
    AssertMessageException(codepoint <= 0x7F);
  } else if (twoBytes) {
    AssertMessageException(offset + 2 <= text.length());
    offset += 2;
    codepoint = ((size_t(text.data()[offset - 2] & 0x1F) << 6) |
                 size_t(text.data()[offset - 1] & 0x3F));
    AssertMessageException((text.data()[offset - 1] & 0xC0) == 0x80);
    AssertMessageException((codepoint > 0x7F) && (codepoint <= 0x7FF));
  } else if (threeBytes) {
    AssertMessageException(offset + 3 <= text.length());
    offset += 3;
    codepoint = ((size_t(text.data()[offset - 3] & 0x0F) << 12) |
                 (size_t(text.data()[offset - 2] & 0x3F) << 6) |
                 size_t(text.data()[offset - 1] & 0x3F));
    AssertMessageException((text.data()[offset - 1] & 0xC0) == 0x80);
    AssertMessageException((text.data()[offset - 2] & 0xC0) == 0x80);
    AssertMessageException((codepoint > 0x7FF) && (codepoint <= 0xFFFF));
  } else if (fourBytes) {
    AssertMessageException(offset + 4 <= text.length());
    offset += 4;
    codepoint = ((size_t(text.data()[offset - 4] & 0x07) << 18) |
                 (size_t(text.data()[offset - 3] & 0x3F) << 12) |
                 (size_t(text.data()[offset - 2] & 0x3F) << 6) |
                 size_t(text.data()[offset - 1] & 0x3F));
    AssertMessageException((text.data()[offset - 1] & 0xC0) == 0x80);
    AssertMessageException((text.data()[offset - 2] & 0xC0) == 0x80);
    AssertMessageException((text.data()[offset - 3] & 0xC0) == 0x80);
    AssertMessageException((codepoint > 0xFFFF) && (codepoint <= 0x10FFFF));
  } else {
    throw std::invalid_argument("invalid codepoint: " + text.substr(offset, 4));
  }
  return codepoint;
}
inline std::string Value::_utf8(size_t codepoint) {
  /*
          1	7	U+0000	U+007F  	0xxxxxxx
          2	11	U+0080	U+07FF  	110xxxxx	10xxxxxx
          3	16	U+0800	U+FFFF  	1110xxxx	10xxxxxx
     10xxxxxx
          4	21	U+10000	U+10FFFF	11110xxx	10xxxxxx
     10xxxxxx	10xxxxxx
  */
  std::string value;

  if (codepoint <= 0x7F) {
    value.assign(1, char(codepoint));
  } else if (codepoint <= 0x7FF) {
    char buffer[3];

    buffer[0] = (6 << 5) | (codepoint >> 6);
    buffer[1] = (2 << 6) | (codepoint & 0x3F);
    buffer[2] = 0;
    value = buffer;
  } else if (codepoint <= 0xFFFF) {
    char buffer[4];

    buffer[0] = (14 << 4) | (codepoint >> 12);
    buffer[1] = (2 << 6) | ((codepoint >> 6) & 0x3F);
    buffer[2] = (2 << 6) | (codepoint & 0x3F);
    buffer[3] = 0;
    value = buffer;
  } else if (codepoint <= 0x10FFFF) {
    char buffer[5];

    buffer[0] = (30 << 3) | (codepoint >> 18);
    buffer[1] = (2 << 6) | ((codepoint >> 12) & 0x3F);
    buffer[2] = (2 << 6) | ((codepoint >> 6) & 0x3F);
    buffer[3] = (2 << 6) | (codepoint & 0x3F);
    buffer[4] = 0;
    value = buffer;
  } else {
    throw std::invalid_argument("invalid codepoint: " +
                                std::to_string(codepoint));
  }
  return value;
}
inline Value::Instance *Value::_create(Type t) {
  switch (t) {
  case ObjectType:
    return new Object();
  case ArrayType:
    return new Array();
  case StringType:
    return new String("");
  case IntegerType:
    return new Integer(0);
  case RealType:
    return new Real(0.0);
  case BooleanType:
    return new Boolean(false);
  case NullType:
  default:
    break;
  }
  return NULL;
}
inline Value &Value::parse(const std::string &text) {
  std::string::size_type offset = 0;
  return parse(text, offset);
}
inline Value &Value::parse(const std::string &text,
                           std::string::size_type &offset) {
  makeNull();
  offset = _skipWhitespace(text, offset);
  if (offset >= text.length()) {
    throw WrongType(std::string("Cannot parse empty string."), __FILE__,
                    __LINE__);
  }
  switch (text[offset]) {
  case '{':
    _parseObject(text, offset);
    break;
  case '[':
    _parseArray(text, offset);
    break;
  case '"':
    _parseString(text, offset);
    break;
  case 't':
    _parseWord(text, "true", offset);
    *this = true;
    break;
  case 'f':
    _parseWord(text, "false", offset);
    *this = false;
    break;
  case 'n':
    _parseWord(text, "null", offset);
    makeNull();
    break;
  default:
    _parseNumber(text, offset);
    break;
  }
  return *this;
}
inline bool Value::boolean() const {
  CheckType(type(), BooleanType);
  return reinterpret_cast<Boolean *>(_value)->value();
}
inline int64_t Value::integer() const {
  CheckType(type(), IntegerType);
  return reinterpret_cast<Integer *>(_value)->value();
}
inline double Value::real() const {
  CheckType(type(), RealType);
  return reinterpret_cast<Real *>(_value)->value();
}
inline const std::string &Value::string() const {
  CheckType(type(), StringType);
  return reinterpret_cast<String *>(_value)->value();
}
inline int Value::count() const {
  const Type t = type();

  switch (t) {
  case ObjectType:
    return reinterpret_cast<Object *>(_value)->count();
  case ArrayType:
    return reinterpret_cast<Array *>(_value)->count();
  case StringType:
    return reinterpret_cast<String *>(_value)->count();
  case NullType:
    break;
  case IntegerType:
  case RealType:
  case BooleanType:
  default:
    Check3Types(t, ObjectType, ArrayType, StringType);
  }
  return 0;
}
inline Value::StringList Value::keys() const {
  CheckType(type(), ObjectType);
  return reinterpret_cast<Object *>(_value)->keys();
}
inline Value &Value::clear() {
  const Type t = type();

  switch (t) {
  case ObjectType:
    reinterpret_cast<Object *>(_value)->clear();
    break;
  case ArrayType:
    reinterpret_cast<Array *>(_value)->clear();
    break;
  case StringType:
    reinterpret_cast<String *>(_value)->clear();
    break;
  case NullType:
    break;
  case IntegerType:
  case RealType:
  case BooleanType:
  default:
    Check3Types(t, ObjectType, ArrayType, StringType);
  }
  return *this;
}
inline Value &Value::erase(int startIndex, int endIndex) {
  const Type t = type();

  switch (t) {
  case ArrayType:
    reinterpret_cast<Array *>(_value)->erase(startIndex, endIndex);
    break;
  case StringType:
    reinterpret_cast<String *>(_value)->erase(startIndex, endIndex);
    break;
  case NullType:
    break;
  case IntegerType:
  case RealType:
  case BooleanType:
  case ObjectType:
  default:
    Check2Types(t, ArrayType, StringType);
  }
  return *this;
}
inline Value &Value::erase(const std::string &key) {
  if (is(NullType)) {
    makeObject();
  }

  CheckType(type(), ObjectType);
  reinterpret_cast<Object *>(_value)->erase(key);
  return *this;
}
inline bool Value::has(const std::string &key) {
  if (is(NullType)) {
    makeObject();
  }

  CheckType(type(), ObjectType);
  return reinterpret_cast<Object *>(_value)->has(key);
}
inline Value &Value::makeObject() {
  if (ObjectType == type()) {
    reinterpret_cast<Object *>(_value)->clear();
  } else {
    makeNull();
    _value = new Object();
  }
  return *this;
}
inline Value &Value::makeArray() {
  if (ArrayType == type()) {
    reinterpret_cast<Array *>(_value)->clear();
  } else {
    makeNull();
    _value = new Array();
  }
  return *this;
}
inline Value &Value::append(const Value &value) {
  if (is(NullType)) {
    makeArray();
  }

  CheckType(type(), ArrayType);
  reinterpret_cast<Array *>(_value)->append(value);
  return *this;
}
/// @todo Test
inline Value &Value::insert(const Value &value, int beforeIndex) {
  if (is(NullType)) {
    makeArray();
  }

  CheckType(type(), ArrayType);
  reinterpret_cast<Array *>(_value)->insert(value, beforeIndex);
  return *this;
}
inline std::string &Value::format(std::string &buffer, int indent,
                                  int indentLevel) const {
  if (NULL == _value) {
    buffer = "null";
  } else {
    _value->format(buffer, indent, indentLevel);
  }
  return buffer;
}
inline std::string Value::format(int indent, int indentLevel) const {
  std::string formatted;

  return format(formatted, indent, indentLevel);
}
inline Value &Value::operator=(int value) {
  *this = (int64_t)value;
  return *this;
}
inline Value &Value::operator=(int64_t value) {
  if (IntegerType == type()) {
    reinterpret_cast<Integer *>(_value)->value() = value;
  } else {
    makeNull();
    _value = new Integer(value);
  }
  return *this;
}
inline Value &Value::operator=(double value) {
  if (RealType == type()) {
    reinterpret_cast<Real *>(_value)->value() = value;
  } else {
    makeNull();
    _value = new Real(value);
  }
  return *this;
}
inline Value &Value::operator=(const std::string &value) {
  if (StringType == type()) {
    reinterpret_cast<String *>(_value)->value() = value;
  } else {
    makeNull();
    _value = new String(value);
  }
  return *this;
}
inline Value &Value::operator=(const char *value) {
  return (*this) = std::string(value);
}
inline Value &Value::operator=(bool value) {
  if (BooleanType == type()) {
    reinterpret_cast<Boolean *>(_value)->value() = value;
  } else {
    makeNull();
    _value = new Boolean(value);
  }
  return *this;
}
inline Value &Value::operator[](int index) {
  CheckType(type(), ArrayType);
  return reinterpret_cast<Array *>(_value)->get(index);
}
inline const Value &Value::operator[](int index) const {
  CheckType(type(), ArrayType);
  return reinterpret_cast<const Array *>(_value)->get(index);
}
inline Value &Value::operator[](const std::string &key) {
  if (is(NullType)) {
    makeObject();
  }

  CheckType(type(), ObjectType);
  return reinterpret_cast<Object *>(_value)->get(key);
}
bool Value::operator==(const Value &other) const {
  if (_value == other._value) {
    return true;
  }
  if ((nullptr == _value) || (nullptr == other._value)) {
    return false; // not tested
  }
  return *_value == *other._value;
}
inline const Value &Value::operator[](const std::string &key) const {
  CheckType(type(), ObjectType);
  return reinterpret_cast<const Object *>(_value)->get(key);
}

inline void Value::String::format(std::string &buffer, int /*indent*/,
                                  int /*indentLevel*/) const {
  size_t codepoint;
  std::string::size_type offset;

  buffer = "\"";
  for (std::string::size_type i = 0; i < _value.length(); ++i) {
    switch (_value[i]) {
    case '\\':
      buffer += "\\\\";
      break;
    case '"':
      buffer += "\\\"";
      break;
    case '/':
      buffer += "\\/";
      break;
    case '\t':
      buffer += "\\t";
      break;
    case '\r':
      buffer += "\\r";
      break;
    case '\n':
      buffer += "\\n";
      break;
    case '\b':
      buffer += "\\b";
      break;
    case '\f':
      buffer += "\\f";
      break;
    default:
      offset = i;
      codepoint = _codepoint(_value, offset);
      if (offset - i == 1) {
        buffer += _value[i];
      } else {
        std::stringstream stream;
        const bool ecma6 = (codepoint > 0xFFFF);

        if (ecma6) { // previous to ecma6, u{xxxxxx} was not supported, keep
                     // utf8 codepoint
          buffer = buffer + _value.substr(i, offset - i);
        } else {
          buffer = buffer + "\\u";
          stream << std::setfill('0') << std::setw(4) << std::hex << codepoint;
          buffer = buffer + stream.str();
        }
      }
      i = offset - 1;
      break;
    }
  }
  buffer += "\"";
}

inline Value::Instance *Value::Array::clone() const {
  Array *value = new Array();

  for (auto i : _value) {
    value->append(i);
  }
  return value;
}
inline void Value::Array::format(std::string &buffer, int indent,
                                 int indentLevel) const {
  std::string value;
  std::string suffix = ",";
  std::string linePrefix = "";
  std::string lastLinePrefix = "";
  std::string lineSuffix = (indent >= 0) ? "\n" : "";

  if (indent >= 0) {
    linePrefix.assign(indent * (indentLevel + 1), ' ');
    lastLinePrefix.assign(indent * indentLevel, ' ');
  }
  buffer = "[" + lineSuffix;
  for (std::vector<Value>::const_iterator i = _value.begin(); i != _value.end();
       ++i) {
    i->format(value, indent, indentLevel + 1);
    if ((i + 1) == _value.end()) {
      suffix = "";
    }
    buffer += linePrefix + value + suffix + lineSuffix;
  }
  buffer += lastLinePrefix + "]";
}

inline bool Value::Array::operator==(const Instance &other) const {
  if (type() != other.type()) {
    return false; // tested in libernet tests
  }

  const Array *array = reinterpret_cast<const Array *>(&other);

  if (_value.size() != array->_value.size()) {
    return false; // not tested
  }

  for (auto i = _value.begin(), j = array->_value.begin(); i != _value.end();
       ++i, ++j) {
    if (!(*i == *j)) {
      return false; // not tested
    }
  }
  return true;
}

inline Value::Instance *Value::Object::clone() const {
  Object *value = new Object();

  for (auto i : _value) {
    value->get(i.first) = i.second;
  }
  return value;
}
inline void Value::Object::format(std::string &buffer, int indent,
                                  int indentLevel) const {
  Value key;
  std::string keyStr;
  std::string value;
  std::string linePrefix = "";
  std::string lastLinePrefix = "";
  std::string lineSuffix = (indent >= 0) ? "\n" : "";
  size_t itemsLeft = _value.size();

  if (indent >= 0) {
    linePrefix.assign(indent * (indentLevel + 1), ' ');
    lastLinePrefix.assign(indent * indentLevel, ' ');
  }
  buffer = "{" + lineSuffix;
  for (auto i : _value) {
    itemsLeft -= 1;
    key = i.first;
    key.format(keyStr, indent, indentLevel + 1);
    i.second.format(value, indent, indentLevel + 1);
    buffer += linePrefix + keyStr + ":" + value + (itemsLeft == 0 ? "" : ",") +
              lineSuffix;
  }
  buffer += lastLinePrefix + "}";
}

inline bool Value::Object::operator==(const Instance &other) const {
  if (type() != other.type()) {
    return false; // not tested
  }
  const Object *object = reinterpret_cast<const Object *>(&other);

  if (_value.size() != object->_value.size()) {
    return false; // tested in libernet tests
  }

  for (auto i : _value) {
    if (!object->has(i.first)) {
      return false; // tested in libernet tests
    }

    if (!(get(i.first) == object->get(i.first))) {
      return false; // tested in libernet tests
    }
  }
  return true;
}

inline Value::StringList Value::Object::keys() const {
  StringList keys;

  for (auto i : _value) {
    // Consider using std::transform algorithm instead of a raw loop
    // cppcheck-suppress useStlAlgorithm
    keys.push_back(i.first);
  }
  return keys;
}
inline const Value &Value::Object::get(const std::string &key) const {
  std::map<std::string, Value>::const_iterator found = _value.find(key);

  if (found == _value.end()) {
    throw WrongType(std::string("Key not found: ") + key, // not tested
                    __FILE__, __LINE__);
  }
  return found->second;
}

// Cleanup
#undef TypeCase
#undef CheckType
#undef Check2Types
#undef Check3Types

} // namespace json

#endif // __JSON_h__
