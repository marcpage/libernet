#ifndef __Karma_h__
#define __Karma_h__

#include "os/Exception.h"
#include <string>

namespace karma {

/// @todo document
/// @todo test
class Karma {
public:
  explicit Karma(const std::string &value);
  explicit Karma(uint64_t karma = 0, uint64_t kismet = 0);
  Karma(const Karma &other) : _karma(other._karma), _kismet(other._kismet) {}
  uint64_t getWholeKarma() const { return _karma; }
  uint64_t getKismet() const { return _kismet; }
  ~Karma() {}
  std::string &string(std::string &buffer);
  std::string string() const {
    std::string buffer;

    return string(buffer);
  }
  operator std::string() const { return string(); }
  Karma operator+(const Karma &other) const { return Karma(*this) += other; }
  Karma &operator+=(const Karma &other);
  Karma operator-(const Karma &other) const { return Karma(*this) -= other; }
  Karma &operator-=(const Karma &other);
  Karma operator*(uint32_t scaler) const { return Karma(*this) *= other; }
  Karma &operator*=(uint32_t scaler);
  Karma operator/(uint32_t scaler) const { return Karma(*this) /= other; }
  Karma &operator/=(uint32_t scaler);
  bool operator==(const Karma &other) const;
  bool operator!=(const Karma &other) const { return !(*this == other); }
  bool operator<(const Karma &other) const;
  bool operator>(const Karma &other) const { return other < *this; }
  bool operator<=(const Karma &other) const { return !(*this > other); }
  bool operator>=(const Karma &other) const { return !(*this < other); }

private:
  uint64_t _karma;
  uint64_t _kismet;
};

inline Karma::Karma(const std::string &value) : _karma(0), _kismet(0) {
  const auto dot = value.find('.');

  _karma = std::stoll(value.substr(0, dot));

  if (std::string::npos != dot) {
    _kismet = std::stoll(value.substr(dot, 14));
  }
}

inline Karma::Karma(uint64_t karma, uint64_t kismet)
    : _karma(karma), _kismet(kismet) {
  const uint64_t oneKarmaInKismet = 100000000000000;

  while (_kismet > oneKarmaInKismet) {
    _karma += 1;
    _kismet -= oneKarmaInKismet;
  }
}

inline std::string &Karma::string(std::string &buffer) {
  buffer.clear();

  if (_kismet > 0) {
    buffer = std::to_string(_kismet);

    while (buffer.size() < 14) {
      buffer = '0' + buffer;
    }

    buffer += '.';
  }

  buffer += std::to_string(_karma);
  return buffer;
}

inline Karma &Karma::operator+=(const Karma &other) {
  const uint64_t oneKarmaInKismet = 100000000000000;

  _karma += other._karma;
  _kismet += other._kismet;

  while (_kismet >= oneKarmaInKismet) {
    _kismet -= oneKarmaInKismet;
    _karma += 1;
  }
  return *this;
}

inline Karma &Karma::operator-=(const Karma &other) {
  const uint64_t oneKarmaInKismet = 100000000000000;

  AssertMessageException(*this >= other);

  while (_kismet < other._kismet) {
    _kismet += oneKarmaInKismet;
    _karma -= 1;
  }

  _kismet -= other._kismet;
  _karma -= other._karma;
  return *this;
}

inline Karma &Karma::operator*=(uint32_t scaler) {
  const uint64_t mask32 = 0x00000000FFFFFFFF;
  const uint64_t oneKarmaInKismet = 100000000000000;
  const uint64_t oneKarmaInKismetH = (oneKarmaInKismet >> 32) & mask32;
  const uint64_t maxKarma = 100000000000000;
  const uint64_t maxKarmaH = (maxKarma >> 32) & mask32;
  const uint64_t kismetL = _kismet & mask32;
  const uint64_t kismetH = (_kismet >> 32) & mask32;
  const uint64_t karmaL = _kismet & mask32;
  const uint64_t karmaH = (_kismet >> 32) & mask32;
  const uint64_t kismetLow = scaler * kismetL;
  const uint64_t kismetHigh = scaler * kismetH;
  const uint64_t karmaLow = scaler * karmaL;
  const uint64_t karmaHigh = scaler * karmaH;
  uint64_t karmaCarry = 0;

  while (kismetHigh > oneKarmaInKismetH) {
    kismetHigh -= oneKarmaInKismetH;
    karmaCarry += 1;
  }

  _kismet = (kismetHigh << 32) + kismetLow;
  AssertMessageException(karmaHigh < maxKarmaH);
  AssertMessageException((((karmaLow + karmaCarry) >> 32) & mask32) <
                         (maxKarmaH - karmaHigh));
  _karma = (karmaHigh << 32) + karmaLow + karmaCarry;
  return *this;
}

inline Karma &Karma::operator/=(uint32_t scaler) {
  AssertMessageException(scaler != 0);
  Karma remainder(*this);
  const Karma denominator(0, scaler);

  *this = 0;
  while (remainder >= denominator) {
    *this += 1;
    remainder -= denominator;
  }
  return *this;
}

inline bool Karma::operator==(const Karma &other) const {
  return (_karma == other._karma) && (_kismet == other._kismet);
}

inline bool Karma::operator<(const Karma &other) const {
  if (_karma == other._karma) {
    return _kismet < other._kismet;
  }
  return _karma < other._karma;
}

} // namespace karma

#endif // __Karma_h__
