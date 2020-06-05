#include "libernet/ServerInformation.h"

#define dotest(condition)                                                      \
  if (!(condition)) {                                                          \
    fprintf(stderr, "FAIL(%s:%d): %s\n", __FILE__, __LINE__, #condition);      \
  }

inline void _testclose(double f1, double f2, const char *const n1,
                       const char *const n2, const char *const file, int line) {
  if (fabs(f1 - f2) > 0.5) { // the should be withing 500 milliseconds
    fprintf(stderr, "FAIL(%s:%d): %s (%0.6f) %s (%0.6f) difference = %0.6f\n",
            file, line, n1, double(f1), n2, double(f2), fabs(f1 - f2));
  }
}

#define testclose(f1, f2) _testclose(f1, f2, #f1, #f2, __FILE__, __LINE__)

int main(const int /*argc*/, const char *const /*argv*/[]) {
  int iterations = 10;
#ifdef __Tracer_h__
  iterations = 1;
#endif
  try {
    std::string identifier1 = hash::sha256("test1").hex();
    std::string identifier2 = hash::sha256("test2").hex();
    std::string identifier3 = hash::sha256("test3").hex();
    dt::DateTime start =
        dt::DateTime(2001, dt::DateTime::Jan, 1, dt::DateTime::GMT);
    printf("start = %0.6f\n", start.operator double());

    for (int i = 0; i < iterations; ++i) {
      data::ServerInformation i1, i2, i3;
      data::JSONData::List list;

      i1.setOwner(identifier1);
      i1.setName("George");
      i1.setAddress("apple.com");
      i1.setPort(80);

      i1.setName(identifier2, "Henry");
      i1.setAddress(identifier2, "127.0.0.1");
      i1.setPort(identifier2, 8000);
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
      i1.setPort(identifier3, 8080);
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
      dotest(i1.port(identifier2) == 8000);
      testclose(start, i1.connection(identifier2,
                                     data::ServerInformation::FirstConnection));
      testclose(
          start + 5.0,
          i1.connection(identifier2, data::ServerInformation::LastConnection));
      dotest(i1.count(identifier2, "connections") == 2);
      dotest(i1.count(identifier2, "failed") == 4);
      dotest(i1.count(identifier2, "time") <
             2 * (dt::DateTime() - start) * 1000 * 1000);
      dotest(i1.count(identifier2, "input") == 2048);
      dotest(i1.count(identifier2, "output") == 2 * 4096);
      dotest(i1.count(identifier2, "response") == 200);
      dotest(i1.count(identifier2, "similar") == 2000);

      dotest(i1.name(identifier3) == "Fred");
      dotest(i1.address(identifier3) == "localhost");
      dotest(i1.port(identifier3) == 8080);
      testclose(
          start + 2.0,
          i1.connection(identifier3, data::ServerInformation::FirstConnection));
      testclose(
          start + 3.0,
          i1.connection(identifier3, data::ServerInformation::LastConnection));
      dotest(i1.count(identifier3, "connections") == 0);
      dotest(i1.count(identifier3, "failed") == 6);
      dotest(i1.count(identifier3, "time") <
             2 * (dt::DateTime() - start) * 1000 * 1000);
      dotest(i1.count(identifier3, "input") == 1024);
      dotest(i1.count(identifier3, "output") == 4096);
      dotest(i1.count(identifier3, "response") == 100);
      dotest(i1.count(identifier3, "similar") == 10000);

      dotest(i2 != i1);

      i2 = i1;
      dotest(i2 == i1);
      dotest(i2.owner() == identifier1);
      dotest(i2.name() == "George");
      dotest(i2.address() == "apple.com");
      dotest(i2.port() == 80);
      dotest(i2.servers(list).size() == 2);

      dotest(i2.name(identifier2) == "Henry");
      dotest(i2.address(identifier2) == "127.0.0.1");
      dotest(i2.port(identifier2) == 8000);
      testclose(start, i2.connection(identifier2,
                                     data::ServerInformation::FirstConnection));
      testclose(
          start + 5.0,
          i1.connection(identifier2, data::ServerInformation::LastConnection));
      dotest(i2.count(identifier2, "connections") == 2);
      dotest(i2.count(identifier2, "failed") == 4);
      dotest(i2.count(identifier2, "time") <
             2 * (dt::DateTime() - start) * 1000 * 1000);
      dotest(i2.count(identifier2, "input") == 2048);
      dotest(i2.count(identifier2, "output") == 2 * 4096);
      dotest(i2.count(identifier2, "response") == 200);
      dotest(i2.count(identifier2, "similar") == 2000);

      dotest(i2.name(identifier3) == "Fred");
      dotest(i2.address(identifier3) == "localhost");
      dotest(i2.port(identifier3) == 8080);
      testclose(
          start + 2.0,
          i2.connection(identifier3, data::ServerInformation::FirstConnection));
      testclose(
          start + 3.0,
          i1.connection(identifier3, data::ServerInformation::LastConnection));
      dotest(i2.count(identifier3, "connections") == 0);
      dotest(i2.count(identifier3, "failed") == 6);
      dotest(i2.count(identifier3, "time") <
             2 * (dt::DateTime() - start) * 1000 * 1000);
      dotest(i2.count(identifier3, "input") == 1024);
      dotest(i2.count(identifier3, "output") == 4096);
      dotest(i2.count(identifier3, "response") == 100);
      dotest(i2.count(identifier3, "similar") == 10000);

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
      dotest(i3.port(identifier2) == 8000);
      testclose(start, i3.connection(identifier2,
                                     data::ServerInformation::FirstConnection));
      testclose(
          start + 5.0,
          i3.connection(identifier2, data::ServerInformation::LastConnection));
      dotest(i3.count(identifier2, "connections") == 2);
      dotest(i3.count(identifier2, "failed") == 4);
      dotest(i3.count(identifier2, "time") <
             2 * (dt::DateTime() - start) * 1000 * 1000);
      dotest(i3.count(identifier2, "input") == 2048);
      dotest(i3.count(identifier2, "output") == 2 * 4096);
      dotest(i3.count(identifier2, "response") == 200);
      dotest(i3.count(identifier2, "similar") == 2000);

      dotest(i3.name(identifier3) == "Fred");
      dotest(i3.address(identifier3) == "localhost");
      dotest(i3.port(identifier3) == 8080);
      testclose(
          start + 2.0,
          i3.connection(identifier3, data::ServerInformation::FirstConnection));
      testclose(
          start + 3.0,
          i3.connection(identifier3, data::ServerInformation::LastConnection));
      dotest(i3.count(identifier3, "connections") == 0);
      dotest(i3.count(identifier3, "failed") == 6);
      dotest(i3.count(identifier3, "time") <
             2 * (dt::DateTime() - start) * 1000 * 1000);
      dotest(i3.count(identifier3, "input") == 1024);
      dotest(i3.count(identifier3, "output") == 4096);
      dotest(i3.count(identifier3, "response") == 100);
      dotest(i3.count(identifier3, "similar") == 10000);

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
      dotest(i4.port(identifier2) == 8000);
      testclose(start, i4.connection(identifier2,
                                     data::ServerInformation::FirstConnection));
      testclose(
          start + 5.0,
          i4.connection(identifier2, data::ServerInformation::LastConnection));
      dotest(i4.count(identifier2, "connections") == 2);
      dotest(i4.count(identifier2, "failed") == 4);
      dotest(i4.count(identifier2, "time") <
             2 * (dt::DateTime() - start) * 1000 * 1000);
      dotest(i4.count(identifier2, "input") == 2048);
      dotest(i4.count(identifier2, "output") == 2 * 4096);
      dotest(i4.count(identifier2, "response") == 200);
      dotest(i4.count(identifier2, "similar") == 2000);

      dotest(i4.name(identifier3) == "Fred");
      dotest(i4.address(identifier3) == "localhost");
      dotest(i4.port(identifier3) == 8080);
      testclose(
          start + 2.0,
          i4.connection(identifier3, data::ServerInformation::FirstConnection));
      testclose(
          start + 3.0,
          i4.connection(identifier3, data::ServerInformation::LastConnection));
      dotest(i4.count(identifier3, "connections") == 0);
      dotest(i4.count(identifier3, "failed") == 6);
      dotest(i4.count(identifier3, "time") <
             2 * (dt::DateTime() - start) * 1000 * 1000);
      dotest(i4.count(identifier3, "input") == 1024);
      dotest(i4.count(identifier3, "output") == 4096);
      dotest(i4.count(identifier3, "response") == 100);
      dotest(i4.count(identifier3, "similar") == 10000);

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
      dotest(i5.port(identifier2) == 8000);
      testclose(start, i5.connection(identifier2,
                                     data::ServerInformation::FirstConnection));
      testclose(
          start + 5.0,
          i5.connection(identifier2, data::ServerInformation::LastConnection));
      dotest(i5.count(identifier2, "connections") == 2);
      dotest(i5.count(identifier2, "failed") == 4);
      dotest(i5.count(identifier2, "time") <
             2 * (dt::DateTime() - start) * 1000 * 1000);
      dotest(i5.count(identifier2, "input") == 2048);
      dotest(i5.count(identifier2, "output") == 2 * 4096);
      dotest(i5.count(identifier2, "response") == 200);
      dotest(i5.count(identifier2, "similar") == 2000);

      dotest(i5.name(identifier3) == "Fred");
      dotest(i5.address(identifier3) == "localhost");
      dotest(i5.port(identifier3) == 8080);
      testclose(
          start + 2.0,
          i5.connection(identifier3, data::ServerInformation::FirstConnection));
      testclose(
          start + 3.0,
          i5.connection(identifier3, data::ServerInformation::LastConnection));
      dotest(i5.count(identifier3, "connections") == 0);
      dotest(i5.count(identifier3, "failed") == 6);
      dotest(i5.count(identifier3, "time") <
             2 * (dt::DateTime() - start) * 1000 * 1000);
      dotest(i5.count(identifier3, "input") == 1024);
      dotest(i5.count(identifier3, "output") == 4096);
      dotest(i5.count(identifier3, "response") == 100);
      dotest(i5.count(identifier3, "similar") == 10000);
    }
  } catch (const std::exception &e) {
    printf("FAIL: Exception: %s\n", e.what());
  }
  return 0;
}
