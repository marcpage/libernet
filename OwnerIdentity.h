#ifndef __OwnedIdentity_h__
#define __OwnedIdentity_h__

#include "os/AsymmetricEncrypt.h"
#include "libernet/Identity.h"
#include "protocol/JSON.h"

namespace data {

class OwnedIdentity : public Identity {
public:
  OwnedIdentity(const int keySize = 4096):_key(keySize, _randomPublicExponent()) {}
  OwnedIdentity(const io::Path &file, const std::string &passphrase);
  OwnedIdentity(const Data &data, const std::string &passphrase);
  OwnedIdentity(const OwnedIdentity &other);
  virtual ~OwnedIdentity() {}
  Identity &operator=(const OwnedIdentity &other) {
  	Identiter::operator=(other);
   	_key = other._key();
   }
  data::Data ownedValue(const std::string &passphrase);
  std::string sign(const std::string &text);
  std::string decrypt(const std::string &encrypted);
  void writeOwned(const io::Path &file, const std::string &passphrase);

private:
  crypt::RSAAES256PrivateKey _key;
  unsigned long _randomPublicExponent();
};

inline OwnedIdentity::OwnedIdentity(const io::Path &file, const std::string &passphrase):_key() {
}
inline OwnedIdentity::OwnedIdentity(const Data &data, const std::string &passphrase):_key() {
}
inline OwnedIdentity::OwnedIdentity(const OwnedIdentity &other):_key() {
}
inline data::Data OwnedIdentity::ownedValue(const std::string &passphrase) {
}
inline std::string OwnedIdentity::sign(const std::string &text) {
}
inline std::string OwnedIdentity::decrypt(const std::string &encrypted) {
}
inline void OwnedIdentity::writeOwned(const io::Path &file, const std::string &passphrase) {
}
inline unsigned long OwnedIdentity::_randomPublicExponent() {
	// public exponents 3, 5, 17, 257 or 65537
	return 3;
}

} // namespace data

#endif // __OwnedIdentity_h__
