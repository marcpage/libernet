#ifndef __OwnedIdentity_h__
#define __OwnedIdentity_h__

#include "libernet/Identity.h"
#include "protocol/JSON.h"

namespace data {

class OwnedIdentity : public Identity {
public:
  OwnedIdentity(const int keySize = 4096);
  OwnedIdentity(const io::Path &file, const std::string &passphrase);
  OwnedIdentity(const Data &data, const std::string &passphrase);
  OwnedIdentity(const OwnedIdentity &other);
  virtual ~OwnedIdentity() {}
  data::Data ownedValue(const std::string &passphrase);
  std::string sign(const std::string &text);
  std::string decrypt(const std::string &encrypted);
  void writeOwned(const io::Path &file, const std::string &passphrase);

private:
  crypt::RSAAES256PrivateKey _key;
};

// public exponents 3, 5, 17, 257 or 65537

} // namespace data

#endif // __OwnedIdentity_h__
