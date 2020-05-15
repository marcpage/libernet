#ifndef __Trust_h__
#define __Trust_h__

#include "libernet/WrapperData.h"

namespace data {

class Trust : public WrapperData {
public:
  Trust() : WrapperData("{\"trust\":{}}") {}
  Trust(const Trust &other) : WrapperData(other) {}
  Trust(const std::string &data, const std::string &identifier,
        const std::string &key)
      : WrapperData(data, identifier, key) {}
  virtual ~Trust() {}
  Trust &operator=(const Trust &other);
  Trust &assign(const std::string &data, const std::string &identifier,
                const std::string &key);
  List &identities(List &trusted, ListAction action = ClearFirsts);
  bool has(const std::string &identity);
  void addTrust(const std::string &identity);
  void addMistaken(const std::string &identity);
  void addDisagree(const std::string &identity);
  void addMalevolent(const std::string &identity);
  void setTimestamp(const std::string &identity,
                    const dt::DateTime &timestamp = dt::DateTime());
  int trust(const std::string &identity);
  int mistaken(const std::string &identity);
  int disagree(const std::string &identity);
  int malevolent(const std::string &identity);
  dt::DateTime timestamp(const std::string &identity);

private:
  void _validate() override;
  std::string _wrapKey() override { return "trust"; }
};

inline Trust &Trust::operator=(const Trust &other) {
  WrapperData::operator=(other);
  return *this;
}

inline Trust &Trust::assign(const std::string &data,
                            const std::string &identifier,
                            const std::string &key) {
  Data::assign(data, identifier, key);
  _validate();
  return *this;
}

inline JSONData::List &Trust::identities(JSONData::List &trusted,
                                         ListAction action) {}
inline bool Trust::has(const std::string &identity) {}
inline void Trust::addTrust(const std::string &identity) {}
inline void Trust::addMistaken(const std::string &identity) {}
inline void Trust::addDisagree(const std::string &identity) {}
inline void Trust::addMalevolent(const std::string &identity) {}
inline void Trust::setTimestamp(const std::string &identity,
                                const dt::DateTime &timestamp) {}
inline int Trust::trust(const std::string &identity) {}
inline int Trust::mistaken(const std::string &identity) {}
inline int Trust::disagree(const std::string &identity) {}
inline int Trust::malevolent(const std::string &identity) {}
inline dt::DateTime Trust::timestamp(const std::string &identity) {}
inline void Trust::_validate() {}

} // namespace data

#endif // __Trust_h__

/*
{
        "timestamp": fractional seconds since epoch,
        "trust": {
                public key identifier: {
                        "trusted": count of times content has been marked as
trusted, "mistaken": count of times content was marked mistaken (honest mistake
suspected) "disagree": count of times content was marked as disagree (maybe not
correct, or maybe correct, but dislike presenation or stance) "malevolent":
count of times contents was marked as malevolent intent "timestamp": optional
field that if set marks that trust information is only valid until this
timestamp
                }
        }
}

The above dictionary is placed in a string in the following wrapper

{
        "trust": string of the above dictionary,
        "signature": signature message,
        "signer": hash of public key,
        "padding": random data to get hash of data to match,
}

*/
