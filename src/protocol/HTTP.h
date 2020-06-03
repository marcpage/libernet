#ifndef __HTTP_h__
#define __HTTP_h__

/** @file HTTP.h
        @todo document
        @todo Test Query with just name, no value (no equals)
        @todo Test parsing a response line with no line endings
        @todo Test response with no protocol version (no number after the slash)
*/

#include <ctype.h>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace http {

class Headers {
public:
  typedef std::string String;
  Headers();
  explicit Headers(const String &text);
  Headers(const String &text, String::size_type &offset);
  Headers(const Headers &other);
  ~Headers() {}
  Headers &operator=(const Headers &other);
  operator String() const;
  const String &operator[](const String &name) const;
  String &operator[](const String &name);
  void remove(const String &name);
  bool has(const String &name) const;
  bool empty() const;

private:
  typedef std::map<String, String> _Headers;
  struct _Range {
    String::size_type start;
    String::size_type end;
    _Range() : start(0), end(0) {}
    _Range(String::size_type s, String::size_type e) : start(s), end(e) {}
    bool empty() { return start == end; }
    String &value(const String &text, String &result) {
      result.assign(text, start, end - start);
      return result;
    }
    void nextLine(const String &text);
    _Range consumeKey(const String &text);
    _Range trimmed(const String &text) const;
  };
  _Headers _headers;
  String::size_type _init(const String &text);
};

class Query {
public:
  enum SearchMode { SearchForQuery, IsQuery };
  typedef std::string String;
  typedef std::vector<String> StringList;
  static String &unescape(const String &value, String &result);
  static String unescape(const String &value);
  static String &escape(const String &value, String &result);
  static String escape(const String &value);
  Query();
  Query(const String &query, SearchMode mode);
  Query(const Query &other);
  ~Query() {}
  Query &operator=(const Query &other);
  const String &operator[](const String &name) const;
  String &operator[](const String &name);
  operator String() const;
  bool hasValue(const String &name) const;
  bool has(const String &name) const;
  const StringList &get(const String &name) const;
  const String &getOne(const String &name) const;
  void remove(const String &name);
  void add(const String &name, const String &value);
  bool empty() const;

private:
  typedef std::map<String, StringList> _KeyValues;
  _KeyValues _keyValues;
};

class RequestLine {
public:
  typedef std::string String;
  RequestLine();
  explicit RequestLine(const String &line);
  RequestLine(const String &line, String::size_type &after);
  RequestLine(const RequestLine &other);
  ~RequestLine() {}
  RequestLine &operator=(const RequestLine &other);
  operator String() const;
  const String &method() const;
  const String &path() const;
  const String &protocol() const;
  const String &version() const;
  const Query &query() const;
  String &method();
  String &path();
  String &protocol();
  String &version();
  Query &query();

private:
  String _method;
  String _path;
  String _protocol;
  String _version;
  Query _query;
  String::size_type _find(bool whitespace, const String &text,
                          String::size_type start, String::size_type end);
  String::size_type _init(const String &line);
};

class ResponseLine {
public:
  typedef std::string String;
  ResponseLine();
  ResponseLine(const ResponseLine &other);
  explicit ResponseLine(const String &line);
  ResponseLine(const String &line, String::size_type &after);
  ~ResponseLine() {}
  ResponseLine &operator=(const ResponseLine &other);
  operator String() const;
  const String &protocol() const;
  const String &version() const;
  const String &code() const;
  const String &message() const;
  String &protocol();
  String &version();
  String &code();
  String &message();

private:
  String _protocol;
  String _version;
  String _code;
  String _message;
  String::size_type _find(bool whitespace, const String &text,
                          String::size_type start, String::size_type end);
  String::size_type _init(const String &line);
};

template <typename HeaderLine> class Message {
public:
  typedef std::string String;
  Message();
  explicit Message(const String &headers);
  Message(const String &headers, String::size_type &after);
  Message(const Message &other);
  ~Message() {}
  Message &operator=(const Message &other);
  operator String() const;
  const HeaderLine &info() const;
  HeaderLine &info();
  const Headers &fields() const;
  Headers &fields();

private:
  HeaderLine _message;
  Headers _headers;
};

/* Headers */

inline Headers::Headers() : _headers() {}
inline Headers::Headers(const String &text) : _headers() { _init(text); }
inline Headers::Headers(const String &text, String::size_type &offset)
    : _headers() {
  offset = _init(text);
}
inline Headers::Headers(const Headers &other) : _headers() { *this = other; }
inline Headers &Headers::operator=(const Headers &other) {
  _headers.clear();
  for (auto i : other._headers) {
    _headers[i.first] = i.second;
  }
  return *this;
}
inline Headers::operator String() const {
  String results("");

  for (auto i : _headers) {
    // Consider using std::accumulate algorithm instead of a raw loop
    // cppcheck-suppress useStlAlgorithm
    results += i.first + ": " + i.second + "\r\n";
  }
  return results + "\r\n";
}
inline const Headers::String &Headers::operator[](const String &name) const {
  _Headers::const_iterator found = _headers.find(name);

  if (found == _headers.end()) {
    throw std::out_of_range(name); // not tested
  }
  return found->second;
}
inline Headers::String &Headers::operator[](const String &name) {
  return _headers[name];
}
inline void Headers::remove(const String &name) { _headers.erase(name); }
inline bool Headers::has(const String &name) const {
  _Headers::const_iterator found = _headers.find(name);

  return (found != _headers.end());
}
inline bool Headers::empty() const { return _headers.empty(); }
inline Headers::String::size_type Headers::_init(const String &text) {
  _Range line;
  String lastKey;
  String value;
  String prefix;

  do {
    line.nextLine(text);

    if (line.end <= line.start) {
      break;
    }

    if (::isspace(text[line.start])) {
      prefix = " ";
    } else {
      line.consumeKey(text).trimmed(text).value(text, lastKey);
      prefix.clear();
    }

    if (!line.trimmed(text).empty() && (lastKey.length() > 0)) {
      _headers[lastKey] += prefix + line.trimmed(text).value(text, value);
    }

  } while (!line.trimmed(text).empty() && (lastKey.length() > 0));
  return line.end;
}
inline void Headers::_Range::nextLine(const String &text) {
  String::size_type cr = text.find('\r', end);
  String::size_type lf = text.find('\n', end);

  start = end;
  if ((cr != String::npos) && (lf != String::npos)) {
    end = (((cr + 1 == lf) || (lf < cr)) ? lf : cr) + 1;
  } else if ((cr != String::npos) || (lf != String::npos)) {
    end = (lf != String::npos ? lf : cr) + 1;
  } else {
    end = text.length();
  }
}
inline Headers::_Range Headers::_Range::consumeKey(const String &text) {
  String::size_type colonPos = text.find(':', start);
  String::size_type keyStart = start;

  if (String::npos == colonPos) {
    return *this; // not tested
  }
  start = colonPos + 1;
  return _Range(keyStart, colonPos);
}
inline Headers::_Range Headers::_Range::trimmed(const String &text) const {
  _Range range(start, end);

  if (end <= start) {
    return range; // not tested
  }
  while ((range.start < range.end) && ::isspace(text[range.start])) {
    ++range.start;
  }
  while ((range.start < range.end) && ::isspace(text[range.end - 1])) {
    --range.end;
  }
  return range;
}

/* Query */

inline Query::String &Query::unescape(const String &value, String &result) {
  const String hex = "0123456789ABCDEF";

  result.clear();
  result.reserve(value.length());
  for (String::size_type i = 0; i < value.length(); ++i) {
    if (('%' == value[i]) && (i + 2 < value.length())) {
      String::size_type upperNibble = hex.find(::toupper(value[i + 1]));
      String::size_type lowerNibble = hex.find(::toupper(value[i + 2]));

      if ((upperNibble != String::npos) && (lowerNibble != String::npos)) {
        result.append(1, (upperNibble << 4) + lowerNibble);
        i += 2;
      } else {
        result.append(1, value[i]);
      }
    } else if ('+' == value[i]) {
      result.append(1, ' ');
    } else {
      result.append(1, value[i]);
    }
  }
  return result;
}
inline Query::String Query::unescape(const String &value) {
  String result;

  return unescape(value, result);
}
inline Query::String &Query::escape(const String &value, String &result) {
  const String hex = "0123456789ABCDEF";

  result.clear();
  result.reserve(value.length());
  for (String::size_type i = 0; i < value.length(); ++i) {
    if (' ' == value[i]) {
      result.append(1, '+');
    } else if (isalnum(value[i])) {
      result.append(1, value[i]);
    } else {
      result.append(1, '%');
      result.append(
          1, hex[reinterpret_cast<const unsigned char &>(value[i]) >> 4]);
      result.append(1, hex[value[i] & 0x0F]);
    }
  }
  return result;
}
inline Query::String Query::escape(const String &value) {
  String result;

  return escape(value, result);
}
inline Query::Query(const String &query, SearchMode mode) : _keyValues() {
  String::size_type queryPos =
      SearchForQuery == mode ? query.find('?') : String::npos;
  String::size_type position = queryPos == String::npos ? 0 : queryPos + 1;
  String::size_type hashPos = query.find('#', position);
  String::size_type end = hashPos == String::npos ? query.length() : hashPos;

  while (position < end) {
    String::size_type separatorPos = query.find('&', position);
    String::size_type positionEnd =
        (separatorPos == String::npos || separatorPos > end) ? end
                                                             : separatorPos;

    if (position < positionEnd) {
      String::size_type equalsPos = query.find('=', position);
      String::size_type keyEndPos =
          equalsPos == positionEnd
              ? positionEnd // not tested
              : (equalsPos > positionEnd ? positionEnd : equalsPos);
      String key = unescape(query.substr(position, keyEndPos - position));
      String value = keyEndPos < positionEnd
                         ? unescape(query.substr(keyEndPos + 1,
                                                 positionEnd - keyEndPos - 1))
                         : String();

      _keyValues[key].push_back(value);
    }
    position = positionEnd + 1;
  }
}
inline Query::Query() : _keyValues() {}
inline Query::Query(const Query &other) : _keyValues() { *this = other; }
inline Query &Query::operator=(const Query &other) {
  _keyValues.clear();
  if (this != &other) {
    for (auto key : other._keyValues) {
      for (auto value : key.second) {
        _keyValues[key.first].push_back(value);
      }
    }
  }
  return *this;
}
inline const Query::String &Query::operator[](const String &name) const {
  return getOne(name);
}
inline Query::String &Query::operator[](const String &name) {
  StringList &values = _keyValues[name];

  if (values.size() == 0) {
    values.push_back("");
  }
  return values[0];
}
inline bool Query::hasValue(const String &name) const {
  _KeyValues::const_iterator found = _keyValues.find(name);

  return (found != _keyValues.end()) && (found->second.size() > 0) &&
         (found->second[0].length() > 0);
}
inline bool Query::has(const String &name) const {
  _KeyValues::const_iterator found = _keyValues.find(name);
  return found != _keyValues.end();
}
inline Query::operator String() const {
  String result = empty() ? "" : "?";
  String prefix = "";

  for (auto key : _keyValues) {
    if (key.second.size() > 0) {
      for (auto value : key.second) {
        result += prefix + escape(key.first);
        if (value.length() > 0) {
          result += "=" + escape(value);
        } else {
        }
        prefix = "&";
      }
    } else { // shouldn't happen. Any time a key is added, at least an empty
             // value should be put in the list
      result += prefix + escape(key.first); // not tested
      prefix = "&";                         // not tested
    }
  }
  return result;
}
inline const Query::StringList &Query::get(const String &name) const {
  _KeyValues::const_iterator found = _keyValues.find(name);

  if (found == _keyValues.end()) {
    throw std::out_of_range(name);
  }
  return found->second;
}
inline const Query::String &Query::getOne(const String &name) const {
  const StringList &found = get(name);

  if (0 == found.size()) { // should never happen. Any time a key is added, at
                           // least an empty value should be put in the list
    throw std::out_of_range(name); // not tested
  }
  return found[0];
}
inline void Query::remove(const String &name) { _keyValues.erase(name); }
inline void Query::add(const String &name, const String &value) {
  _keyValues[name].push_back(value);
}
inline bool Query::empty() const { return _keyValues.empty(); }

/* RequestLine */

inline RequestLine::RequestLine()
    : _method("GET"), _path("/"), _protocol("HTTP"), _version("1.1"), _query() {
}
inline RequestLine::RequestLine(const String &line)
    : _method(), _path(), _protocol(), _version("1.1"), _query() {
  _init(line);
}
inline RequestLine::RequestLine(const String &line, String::size_type &after)
    : _method(), _path(), _protocol(), _version("1.1"), _query() {
  after = _init(line);
}
inline RequestLine::RequestLine(const RequestLine &other)
    : _method(other._method), _path(other._path), _protocol(other._protocol),
      _version(other._version), _query(other._query) {}
inline RequestLine &RequestLine::operator=(const RequestLine &other) {
  _method = other._method;
  _path = other._path;
  _protocol = other._protocol;
  _version = other._version;
  _query = other._query;
  return *this;
}
inline RequestLine::operator String() const {
  return _method + " " + _path + std::string(_query) + " " + _protocol + "/" +
         _version;
}
inline const RequestLine::String &RequestLine::method() const {
  return _method;
}
inline const RequestLine::String &RequestLine::path() const { return _path; }
inline const RequestLine::String &RequestLine::protocol() const {
  return _protocol;
}
inline const RequestLine::String &RequestLine::version() const {
  return _version;
}
inline const Query &RequestLine::query() const { return _query; }
inline RequestLine::String &RequestLine::method() { return _method; }
inline RequestLine::String &RequestLine::path() { return _path; }
inline RequestLine::String &RequestLine::protocol() { return _protocol; }
inline RequestLine::String &RequestLine::version() { return _version; }
inline Query &RequestLine::query() { return _query; }
inline RequestLine::String::size_type
RequestLine::_find(bool whitespace, const String &text, String::size_type start,
                   String::size_type end) {
  while ((start < end) &&
         ((::isspace(text[start]) ? true : false) != whitespace)) {
    ++start;
  }
  return start;
}
inline RequestLine::String::size_type RequestLine::_init(const String &line) {
  String::size_type start = 0;
  String::size_type end;
  String::size_type queryPos;
  String::size_type slashPos;
  String::size_type lineEnd = line.find('\r');
  String::size_type after;

  if (String::npos == lineEnd) {
    lineEnd = line.find('\n');
  }
  after =
      lineEnd + (((String::npos != lineEnd) && (line[lineEnd] == '\r') &&
                  (lineEnd + 1 < line.length()) && (line[lineEnd + 1] == '\n'))
                     ? 1
                     : 0);
  if (String::npos == lineEnd) {
    lineEnd = line.length();
    after = lineEnd;
  }
  end = _find(true, line, start, lineEnd);
  _method = line.substr(start, end - start);
  start = _find(false, line, end, lineEnd);
  end = _find(true, line, start, lineEnd);
  queryPos = line.find('?', start);
  if ((queryPos == String::npos) || (queryPos > end)) {
    _path = line.substr(start, end - start);
  } else {
    _path = line.substr(start, queryPos - start);
    _query =
        Query(line.substr(queryPos + 1, end - queryPos - 1), Query::IsQuery);
  }
  start = _find(false, line, end, lineEnd);
  end = _find(true, line, start, lineEnd);
  slashPos = line.find('/', start);
  if ((slashPos == String::npos) || (slashPos > end)) {
    _protocol = line.substr(start, end - start);
  } else {
    _protocol = line.substr(start, slashPos - start);
    _version = line.substr(slashPos + 1, end - slashPos - 1);
  }
  return after + 1;
}

/* ResponseLine */

inline ResponseLine::ResponseLine()
    : _protocol("HTTP"), _version("1.0"), _code("404"),
      _message("File Not Found") {}
inline ResponseLine::ResponseLine(const ResponseLine &other)
    : _protocol(other._protocol), _version(other._version), _code(other._code),
      _message(other._message) {}
inline ResponseLine::ResponseLine(const String &line)
    : _protocol(), _version(), _code(), _message() {
  _init(line);
}
inline ResponseLine::ResponseLine(const String &line, String::size_type &after)
    : _protocol(), _version(), _code(), _message() {
  after = _init(line);
}
inline ResponseLine &ResponseLine::operator=(const ResponseLine &other) {
  _code = other._code;
  _message = other._message;
  _protocol = other._protocol;
  _version = other._version;
  return *this;
}
inline ResponseLine::operator String() const {
  return _protocol + "/" + _version + " " + _code + " " + _message;
}
inline const ResponseLine::String &ResponseLine::protocol() const {
  return _protocol;
}
inline const ResponseLine::String &ResponseLine::version() const {
  return _version;
}
inline const ResponseLine::String &ResponseLine::code() const { return _code; }
inline const ResponseLine::String &ResponseLine::message() const {
  return _message;
}
inline ResponseLine::String &ResponseLine::protocol() { return _protocol; }
inline ResponseLine::String &ResponseLine::version() { return _version; }
inline ResponseLine::String &ResponseLine::code() { return _code; }
inline ResponseLine::String &ResponseLine::message() { return _message; }
inline ResponseLine::String::size_type
ResponseLine::_find(bool whitespace, const String &text,
                    String::size_type start, String::size_type end) {
  while ((start < end) &&
         ((::isspace(text[start]) ? true : false) != whitespace)) {
    ++start;
  }
  return start;
}
inline ResponseLine::String::size_type ResponseLine::_init(const String &line) {
  String::size_type start = 0;
  String::size_type end;
  String::size_type lineEnd = line.find('\r');
  String::size_type after;

  if (String::npos == lineEnd) {
    lineEnd = line.find('\n');
  }
  after =
      lineEnd + (((line[lineEnd] == '\r') && (lineEnd + 1 < line.length()) &&
                  (line[lineEnd + 1] == '\n'))
                     ? 2
                     : 1);
  if (String::npos == lineEnd) {
    lineEnd = line.length();
    after = lineEnd;
  }
  end = _find(true, line, start, lineEnd);
  if (String::npos != end) {
    String::size_type slashPos = line.find('/');

    if (String::npos == slashPos) {
      _protocol = line.substr(start, end - start);
      _version = "1.0";
    } else {
      _protocol = line.substr(start, slashPos - start);
      _version = line.substr(slashPos + 1, end - slashPos - 1);
    }
  }
  start = _find(false, line, end, lineEnd);
  end = _find(true, line, start, lineEnd);
  _code = line.substr(start, end - start);
  start = _find(false, line, end, lineEnd);
  _message = line.substr(start, lineEnd - start);
  return after;
}

/* Message */

template <typename HeaderLine>
inline Message<HeaderLine>::Message() : _message(), _headers() {} // not tested
template <typename HeaderLine>
inline Message<HeaderLine>::Message(const String &headers)
    : _message(), _headers() {
  String::size_type offset = 0;
  _message = HeaderLine(headers, offset);
  _headers = Headers(headers.substr(offset));
}
/// @todo Test
template <typename HeaderLine>
inline Message<HeaderLine>::Message(const String &headers,
                                    String::size_type &after)
    : _message(), _headers() {
  _message = HeaderLine(headers, after);
  _headers = Headers(headers.substr(after), after);
}
template <typename HeaderLine>
inline Message<HeaderLine>::Message(const Message<HeaderLine> &other)
    : _message(other._message), _headers(other._headers) {}
template <typename HeaderLine>
inline Message<HeaderLine> &
Message<HeaderLine>::operator=(const Message<HeaderLine> &other) {
  _message = other._message;
  _headers = other._headers;
  return *this;
}
template <typename HeaderLine>
inline Message<HeaderLine>::operator String() const {
  return String(_message) + "\r\n" + String(_headers);
}
/// @todo Test
template <typename HeaderLine>
inline const HeaderLine &Message<HeaderLine>::info() const {
  return _message;
}
template <typename HeaderLine> inline HeaderLine &Message<HeaderLine>::info() {
  return _message;
}
/// @todo Test
template <typename HeaderLine>
inline const Headers &Message<HeaderLine>::fields() const {
  return _headers;
}
template <typename HeaderLine> inline Headers &Message<HeaderLine>::fields() {
  return _headers;
}

typedef Message<RequestLine> Request;
typedef Message<ResponseLine> Response;

} // namespace http

#endif // __HTTP_h__
