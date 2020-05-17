#include "libernet/ServerInformation.h"

#define dotest(condition)                                                      \
  if (!(condition)) {                                                          \
    fprintf(stderr, "FAIL(%s:%d): %s\n", __FILE__, __LINE__, #condition);      \
  }

int main(const int /*argc*/, const char *const /*argv*/[]) {
  int iterations = 2;
#ifdef __Tracer_h__
  iterations = 1;
#endif
  try {
    std::string identifier1 = dummy("test1").identifier();
    std::string identifier2 = dummy("test2").identifier();
    std::string identifier3 = dummy("test3").identifier();
  dt:
    DateTime start;

    for (int i = 0; i < iterations; ++i) {
      data::ServerInformation i1, i2, i3;
      data::JSONData::List list;

      i1.setOwner(identifier1);
      i1.setName("George");
      i1.setAddress("apple.com");
      i1.setPort(80);

      i1.setName(identifier2, "Henry");
      i1.setAddress(identifier2, "127.0.0.1");
      i1.setPort(identitifier2, 8000);
      i1.setConnection(identifier2, start,
                       data::ServerInformation::FirstConnection);
      i1.setConnection(identifier2, start + 5.0,
                       data::ServerInformation::LastConnection);
      i1.increment(identifier2, "connections");
      i1.increment(identifier2, "failed", 2);
      i1.increment(identifier2, "time", (dt::DateTime() - start) * 1000 * 1000);
      i1.increment(identifier2, "input", 1024);
      i1.increment(identifier2, "output", 4096);
      i1.increment(identifier2, "response", 100);
      i1.increment(identifier2, "similar", 1000);
      i1.increment(identifier2, "connections");
      i1.increment(identifier2, "failed", 2);
      i1.increment(identifier2, "time", (dt::DateTime() - start) * 1000 * 1000);
      i1.increment(identifier2, "input", 1024);
      i1.increment(identifier2, "output", 4096);
      i1.increment(identifier2, "response", 100);
      i1.increment(identifier2, "similar", 1000);

      i1.setName(identifier3, "Fred");
      i1.setAddress(identifier3, "localhost");
      i1.setPort(identitifier2, 8080);
      i1.setConnection(identifier3, start + 2.0,
                       data::ServerInformation::FirstConnection);
      i1.setConnection(identifier3, start + 3.0,
                       data::ServerInformation::LastConnection);
      i1.increment(identifier3, "connections", 0);
      i1.increment(identifier3, "failed", 3);
      i1.increment(identifier3, "time", (dt::DateTime() - start) * 1000 * 1000);
      i1.increment(identifier3, "input", 512);
      i1.increment(identifier3, "output", 2048);
      i1.increment(identifier3, "response", 50);
      i1.increment(identifier3, "similar", 5000);
      i1.increment(identifier3, "connections", 0);
      i1.increment(identifier3, "failed", 3);
      i1.increment(identifier3, "time", (dt::DateTime() - start) * 1000 * 1000);
      i1.increment(identifier3, "input", 512);
      i1.increment(identifier3, "output", 2048);
      i1.increment(identifier3, "response", 50);
      i1.increment(identifier3, "similar", 5000);

      dotest(i1.owner() == identifier1);
      dotest(i1.name() == "George");
      dotest(i1.address() == "apple.com");
      dotest(i1.port() == 80);
      dotest(i1.servers(list).size() == 2);

      dotest(i1.name(identifier2) == "Henry");
      dotest(i1.address(identifier2) == "127.0.0.1");
      dotest(i1.port() == 8000);
      dotest(fabs(start - i1.connection(
                              identifier2,
                              data::ServerInformation::FirstConnection)) < 1.0);
      dotest(fabs(5.0 + start -
                  i1.connection(identifier2,
                                data::ServerInformation::LastConnection)) <
             1.0);
      dotest(i1.count("connections") == 2);
      dotest(i1.count("failed") == 4);
      dotest(i1.count("time") < 2 * (dt::DateTime() - start) * 1000 * 1000);
      dotest(i1.count("input") == 2048);
      dotest(i1.count("output") == 2 * 4096);
      dotest(i1.count("response") == 200);
      dotest(i1.count("similar") == 2000);

      dotest(i1.name(identifier2) == "Fred");
      dotest(i1.address(identifier2) == "localhost");
      dotest(i1.port() == 8080);
      dotest(fabs(2.0 + start -
                  i1.connection(identifier2,
                                data::ServerInformation::FirstConnection)) <
             1.0);
      dotest(fabs(3.0 + start -
                  i1.connection(identifier2,
                                data::ServerInformation::LastConnection)) <
             1.0);
      dotest(i1.count("connections") == 0);
      dotest(i1.count("failed") == 6);
      dotest(i1.count("time") < 2 * (dt::DateTime() - start) * 1000 * 1000);
      dotest(i1.count("input") == 1024);
      dotest(i1.count("output") == 4096);
      dotest(i1.count("response") == 100);
      dotest(i1.count("similar") == 10000);

      dotest(i2 != i1);
      dotest(i2 == i1);

      i2 = i1;
      dotest(i2 == i1);
      dotest(i2.owner() == identifier1);
      dotest(i2.name() == "George");
      dotest(i2.address() == "apple.com");
      dotest(i2.port() == 80);
      dotest(i2.servers(list).size() == 2);

      dotest(i2.name(identifier2) == "Henry");
      dotest(i2.address(identifier2) == "127.0.0.1");
      dotest(i2.port() == 8000);
      dotest(fabs(start - i2.connection(
                              identifier2,
                              data::ServerInformation::FirstConnection)) < 1.0);
      dotest(fabs(5.0 + start -
                  i2.connection(identifier2,
                                data::ServerInformation::LastConnection)) <
             1.0);
      dotest(i2.count("connections") == 2);
      dotest(i2.count("failed") == 4);
      dotest(i2.count("time") < 2 * (dt::DateTime() - start) * 1000 * 1000);
      dotest(i2.count("input") == 2048);
      dotest(i2.count("output") == 2 * 4096);
      dotest(i2.count("response") == 200);
      dotest(i2.count("similar") == 2000);

      dotest(i2.name(identifier2) == "Fred");
      dotest(i2.address(identifier2) == "localhost");
      dotest(i2.port() == 8080);
      dotest(fabs(2.0 + start -
                  i2.connection(identifier2,
                                data::ServerInformation::FirstConnection)) <
             1.0);
      dotest(fabs(3.0 + start -
                  i2.connection(identifier2,
                                data::ServerInformation::LastConnection)) <
             1.0);
      dotest(i2.count("connections") == 0);
      dotest(i2.count("failed") == 6);
      dotest(i2.count("time") < 2 * (dt::DateTime() - start) * 1000 * 1000);
      dotest(i2.count("input") == 1024);
      dotest(i2.count("output") == 4096);
      dotest(i2.count("response") == 100);
      dotest(i2.count("similar") == 10000);

      i3.assign(i2.data(), i2.identifier());
      dotest(i3 == i2);
      dotest(i3 == i1);
      dotest(i3.owner() == identifier1);
      dotest(i3.name() == "George");
      dotest(i3.address() == "apple.com");
      dotest(i3.port() == 80);
      dotest(i3.servers(list).size() == 2);

      dotest(i3.name(identifier2) == "Henry");
      dotest(i3.address(identifier2) == "127.0.0.1");
      dotest(i3.port() == 8000);
      dotest(fabs(start - i3.connection(
                              identifier2,
                              data::ServerInformation::FirstConnection)) < 1.0);
      dotest(fabs(5.0 + start -
                  i3.connection(identifier2,
                                data::ServerInformation::LastConnection)) <
             1.0);
      dotest(i3.count("connections") == 2);
      dotest(i3.count("failed") == 4);
      dotest(i3.count("time") < 2 * (dt::DateTime() - start) * 1000 * 1000);
      dotest(i3.count("input") == 2048);
      dotest(i3.count("output") == 2 * 4096);
      dotest(i3.count("response") == 200);
      dotest(i3.count("similar") == 2000);

      dotest(i3.name(identifier2) == "Fred");
      dotest(i3.address(identifier2) == "localhost");
      dotest(i3.port() == 8080);
      dotest(fabs(2.0 + start -
                  i3.connection(identifier2,
                                data::ServerInformation::FirstConnection)) <
             1.0);
      dotest(fabs(3.0 + start -
                  i3.connection(identifier2,
                                data::ServerInformation::LastConnection)) <
             1.0);
      dotest(i3.count("connections") == 0);
      dotest(i3.count("failed") == 6);
      dotest(i3.count("time") < 2 * (dt::DateTime() - start) * 1000 * 1000);
      dotest(i3.count("input") == 1024);
      dotest(i3.count("output") == 4096);
      dotest(i3.count("response") == 100);
      dotest(i3.count("similar") == 10000);

      data::ServerInformation i4(i3);
      dotest(i4 == i3);
      dotest(i4 == i2);
      dotest(i4 == i1);
      dotest(i4.owner() == identifier1);
      dotest(i4.name() == "George");
      dotest(i4.address() == "apple.com");
      dotest(i4.port() == 80);
      dotest(i4.servers(list).size() == 2);

      dotest(i4.name(identifier2) == "Henry");
      dotest(i4.address(identifier2) == "127.0.0.1");
      dotest(i4.port() == 8000);
      dotest(fabs(start - i4.connection(
                              identifier2,
                              data::ServerInformation::FirstConnection)) < 1.0);
      dotest(fabs(5.0 + start -
                  i4.connection(identifier2,
                                data::ServerInformation::LastConnection)) <
             1.0);
      dotest(i4.count("connections") == 2);
      dotest(i4.count("failed") == 4);
      dotest(i4.count("time") < 2 * (dt::DateTime() - start) * 1000 * 1000);
      dotest(i4.count("input") == 2048);
      dotest(i4.count("output") == 2 * 4096);
      dotest(i4.count("response") == 200);
      dotest(i4.count("similar") == 2000);

      dotest(i4.name(identifier2) == "Fred");
      dotest(i4.address(identifier2) == "localhost");
      dotest(i4.port() == 8080);
      dotest(fabs(2.0 + start -
                  i4.connection(identifier2,
                                data::ServerInformation::FirstConnection)) <
             1.0);
      dotest(fabs(3.0 + start -
                  i4.connection(identifier2,
                                data::ServerInformation::LastConnection)) <
             1.0);
      dotest(i4.count("connections") == 0);
      dotest(i4.count("failed") == 6);
      dotest(i4.count("time") < 2 * (dt::DateTime() - start) * 1000 * 1000);
      dotest(i4.count("input") == 1024);
      dotest(i4.count("output") == 4096);
      dotest(i4.count("response") == 100);
      dotest(i4.count("similar") == 10000);

      data::ServerInformation i5(i4.data(), i4.identifier());
      dotest(i5 == i4);
      dotest(i5 == i3);
      dotest(i5 == i2);
      dotest(i5 == i1);
      dotest(i5.owner() == identifier1);
      dotest(i5.name() == "George");
      dotest(i5.address() == "apple.com");
      dotest(i5.port() == 80);
      dotest(i5.servers(list).size() == 2);

      dotest(i5.name(identifier2) == "Henry");
      dotest(i5.address(identifier2) == "127.0.0.1");
      dotest(i5.port() == 8000);
      dotest(fabs(start - i5.connection(
                              identifier2,
                              data::ServerInformation::FirstConnection)) < 1.0);
      dotest(fabs(5.0 + start -
                  i5.connection(identifier2,
                                data::ServerInformation::LastConnection)) <
             1.0);
      dotest(i5.count("connections") == 2);
      dotest(i5.count("failed") == 4);
      dotest(i5.count("time") < 2 * (dt::DateTime() - start) * 1000 * 1000);
      dotest(i5.count("input") == 2048);
      dotest(i5.count("output") == 2 * 4096);
      dotest(i5.count("response") == 200);
      dotest(i5.count("similar") == 2000);

      dotest(i5.name(identifier2) == "Fred");
      dotest(i5.address(identifier2) == "localhost");
      dotest(i5.port() == 8080);
      dotest(fabs(2.0 + start -
                  i5.connection(identifier2,
                                data::ServerInformation::FirstConnection)) <
             1.0);
      dotest(fabs(3.0 + start -
                  i5.connection(identifier2,
                                data::ServerInformation::LastConnection)) <
             1.0);
      dotest(i5.count("connections") == 0);
      dotest(i5.count("failed") == 6);
      dotest(i5.count("time") < 2 * (dt::DateTime() - start) * 1000 * 1000);
      dotest(i5.count("input") == 1024);
      dotest(i5.count("output") == 4096);
      dotest(i5.count("response") == 100);
      dotest(i5.count("similar") == 10000);
    }
  } catch (const std::exception &e) {
    printf("FAIL: Exception: %s\n", e.what());
  }
  return 0;
}
