#include "protocol/JSON.h"
#include <math.h>
#include <stdio.h>

#define dotest(condition)                                                      \
  if (!(condition)) {                                                          \
    fprintf(stderr, "FAIL(%s:%d): %s\n", __FILE__, __LINE__, #condition);      \
  }

void testConstArray(const json::Value &array, int index,
                    const json::Value &value) {
  dotest(array[index] == value);
}

void testConstObject(const json::Value &object, const std::string &key,
                     const json::Value &value) {
  auto list = object.keys();
  bool found = false;

  for (auto i = list.begin(); i != list.end(); ++i) {
    if (*i == key) {
      found = true;
    }
  }
  dotest(found);
  dotest(object[key] == value);
}

int main(int /*argc*/, char * /*argv*/[]) {
  int iterations = 500;
#ifdef __Tracer_h__
  iterations = 1;
#endif
  for (int i = 0; i < iterations; ++i) {
    try {
      const char *j1 = "{\"test "
                       "\\\"me\\\"\":[1,2,{\"go\\/"
                       "now\":3},{\"eol\":\"\\r\\n\"},{\"ht\":\"\\t\"},{\"vt\":"
                       "\"\\f\"},true,false,null,\"C:\\\\\"]}";

      printf("-=- original -=-\n'%s'\n-=- parsed -=-\n'%s'\n", j1,
             json::Value().parse(j1).format().c_str());
      dotest(json::Value().parse(j1).format() == j1);
      dotest(json::Value().parse(j1).is(json::ObjectType));
      dotest(json::Value().parse(j1)["test \"me\""][9].count() == 3);
      dotest(json::Value().parse(j1)["test \"me\""].is(json::ArrayType));
      dotest(json::Value().parse(j1)["test \"me\""][0].is(json::IntegerType));
      dotest(json::Value().parse(j1)["test \"me\""][1].is(json::IntegerType));
      dotest(json::Value().parse(j1)["test \"me\""][2].is(json::ObjectType));
      dotest(json::Value().parse(j1)["test \"me\""][2]["go/now"].is(
          json::IntegerType));
      dotest(json::Value().parse(j1)["test \"me\""][3].is(json::ObjectType));
      dotest(json::Value().parse(j1)["test \"me\""][3]["eol"].is(
          json::StringType));
      dotest(json::Value().parse(j1)["test \"me\""][4].is(json::ObjectType));
      dotest(
          json::Value().parse(j1)["test \"me\""][4]["ht"].is(json::StringType));
      dotest(json::Value().parse(j1)["test \"me\""][5].is(json::ObjectType));
      dotest(
          json::Value().parse(j1)["test \"me\""][5]["vt"].is(json::StringType));
      dotest(json::Value().parse(j1)["test \"me\""][6].is(json::BooleanType));
      dotest(json::Value().parse(j1)["test \"me\""][7].is(json::BooleanType));
      dotest(json::Value().parse(j1)["test \"me\""][8].is(json::NullType));
      dotest(json::Value().parse(j1)["test \"me\""][9].is(json::StringType));
      dotest(json::Value().count() == 0);
      dotest(json::Value(json::NullType).is(json::NullType));
      dotest(json::Value(json::ObjectType).is(json::ObjectType));
      dotest(json::Value(json::ObjectType).count() == 0);
      dotest(json::Value(json::ArrayType).is(json::ArrayType));
      dotest(json::Value(json::ArrayType).count() == 0);
      dotest(json::Value(json::IntegerType).is(json::IntegerType));
      dotest(json::Value(json::IntegerType).integer() == 0);
      dotest(json::Value(json::BooleanType).is(json::BooleanType));
      dotest(json::Value(json::BooleanType).boolean() == false);
      dotest(json::Value(json::RealType).is(json::RealType));
      dotest(json::Value(json::RealType).real() == 0.0);
      dotest(json::Value(json::StringType).is(json::StringType));
      dotest(json::Value(json::StringType).string() == "");

      json::Value test6;
      test6["test"] = "me";
      dotest(test6.is(json::ObjectType));
      dotest(test6["test"].is(json::StringType));
      dotest(test6["test"].string() == "me");
      testConstObject(test6, "test", json::Value() = "me");

      json::Value test7;
      test7.erase("key");
      dotest(test7.is(json::ObjectType));

      json::Value test8;
      dotest(!test8.has("key"));
      dotest(test8.is(json::ObjectType));

      json::Value test9;
      test9.append(json::Value() = "me");
      dotest(test9.is(json::ArrayType));
      dotest(test9.count() == 1);
      dotest(test9[0].string() == "me");
      testConstArray(test9, 0, json::Value() = "me");
      try {
        json::Value().parse(j1)["test \"me\""][0].count();
        dotest(false);
      } catch (const json::WrongType &) {
      }
      try {
        json::Value().parse(j1).boolean();
        dotest(false);
      } catch (const json::WrongType &) {
      }
      try {
        json::Value().parse(j1).real();
        dotest(false);
      } catch (const json::WrongType &) {
      }
      try {
        json::Value().parse(j1).integer();
        dotest(false);
      } catch (const json::WrongType &) {
      }
      try {
        json::Value().parse(j1).string();
        dotest(false);
      } catch (const json::WrongType &) {
      }
      try {
        json::Value().string();
        dotest(false);
      } catch (const json::WrongType &) {
      }
      try {
        json::Value().parse(j1)["test \"me\""].string();
        dotest(false);
      } catch (const json::WrongType &) {
      }
      try {
        json::Value().parse("\"\\u123k\"");
        dotest(false);
      } catch (const json::WrongType &) {
      }
      try {
        json::Value().parse("\"\\u{1\"");
        dotest(false);
      } catch (const json::WrongType &) {
      }
      try {
        json::Value().parse("\"\\u{}\"");
        dotest(false);
      } catch (const json::WrongType &) {
      }
      try {
        json::Value().parse("\"\\u{123k}\"");
        dotest(false);
      } catch (const json::WrongType &) {
      }
      try {
        json::Value().parse("=");
        dotest(false);
      } catch (const json::WrongType &) {
      }
      try {
        json::Value().parse("....");
        dotest(false);
      } catch (const json::WrongType &) {
      }
      try {
        json::Value().parse("\"\xF8\"").format();
        dotest(false);
      } catch (const std::invalid_argument &) {
      }
      printf("--------------------------------\n");
      try {
        json::Value().parse("\"\\u{110000}\"");
        dotest(false);
      } catch (const std::invalid_argument &) {
      }
      printf("--------------------------------\n");
      try {
        json::Value().parse("\"\xc0\x80\"").format();
        dotest(false);
      } catch (const msg::Exception &) {
      }
      try {
        json::Value().parse("\"\xc0\x00\"").format();
        dotest(false);
      } catch (const msg::Exception &) {
      }
      try {
        json::Value().parse("\"\xe0\x80\x80\"").format();
        dotest(false);
      } catch (const msg::Exception &) {
      }
      try {
        json::Value().parse("\"\xe0\x00\x80\"").format();
        dotest(false);
      } catch (const msg::Exception &) {
      }
      try {
        json::Value().parse("\"\xe0\x80\x00\"").format();
        dotest(false);
      } catch (const msg::Exception &) {
      }
      try {
        json::Value().parse("\"\xf0\x00\x80\x80\"").format();
        dotest(false);
      } catch (const msg::Exception &) {
      }
      try {
        json::Value().parse("\"\xf0\x80\x00\x80\"").format();
        dotest(false);
      } catch (const msg::Exception &) {
      }
      try {
        json::Value().parse("\"\xf0\x80\x80\x00\"").format();
        dotest(false);
      } catch (const msg::Exception &) {
      }
      dotest(json::Value().parse("\"\\u0041\"").string() == "A");
      dotest(json::Value().parse("\"\\u00Ae\"").string() == "\xc2\xae");
      dotest(json::Value().parse("\"\\u2211\"").string() == "\xe2\x88\x91");
      dotest(json::Value().parse("\"\\u{1f605}\"").string() ==
             "\xf0\x9f\x98\x85");
      printf("Here we go\n");
      dotest(json::Value().parse("\"\xc2\xae\"").string() == "\xc2\xae");
      dotest(json::Value().parse("\"\xe2\x88\x91\"").string() ==
             "\xe2\x88\x91");
      dotest(json::Value().parse("\"\xf0\x9f\x98\x85\"").string() ==
             "\xf0\x9f\x98\x85");
      printf("%s\n", json::Value().parse("\"\xc2\xae\"").format().c_str());
      printf("%s\n", json::Value().parse("\"\xe2\x88\x91\"").format().c_str());
      dotest(json::Value().parse("\"\xc2\xae\"").format() == "\"\\u00ae\"");
      dotest(json::Value().parse("\"\xe2\x88\x91\"").format() == "\"\\u2211\"");
      dotest(json::Value().parse("\"\xf0\x9f\x98\x85\"").format() ==
             "\"\xf0\x9f\x98\x85\"");

      const char *j2 = " \t{\t \"real\"\t: 3.14159265, \"true\": true, "
                       "\"false\": false, \"array\" : [ 1, 2, 3, 4, 5 ]} ";
      json::Value json2(j2);

      printf("-=- original -=-\n'%s'\n-=- parsed -=-\n'%s'\n", j2,
             json2.format().c_str());
      dotest(::fabs(json2["real"].real() - 3.14159265) < 0.0000001);
      dotest(json2["true"].boolean());
      testConstObject(json2, "true", json::Value() = true);
      dotest(!json2["false"].boolean());
      testConstObject(json2, "false", json::Value() = false);
      dotest(json2["array"].count() == 5);
      dotest(json2["array"][0].integer() == 1);
      dotest(json2["array"][1].integer() == 2);
      dotest(json2["array"][2].integer() == 3);
      dotest(json2["array"][3].integer() == 4);
      dotest(json2["array"][4].integer() == 5);
      json2["array"].erase(4, 5);
      dotest(json2["array"].count() == 4);
      dotest(json2["array"][0].integer() == 1);
      dotest(json2["array"][1].integer() == 2);
      dotest(json2["array"][2].integer() == 3);
      dotest(json2["array"][3].integer() == 4);
      json2["array"].erase(0, 1);
      dotest(json2["array"].count() == 3);
      dotest(json2["array"][0].integer() == 2);
      dotest(json2["array"][1].integer() == 3);
      dotest(json2["array"][2].integer() == 4);
      json2["array"].erase(1, 2);
      dotest(json2["array"].count() == 2);
      dotest(json2["array"][0].integer() == 2);
      dotest(json2["array"][1].integer() == 4);
      json2["array"].clear();
      dotest(json2["array"].count() == 0);
      dotest(json2.count() == 4);
      json2.erase("false");
      dotest(json2.count() == 3);
      json2.clear();
      dotest(json2.count() == 0);
      json2["hello"] = std::string("yes");
      dotest(json2["hello"].is(json::StringType));
      dotest(json2["hello"].string() == "yes");
      dotest(json2["hello"].count() == 3);
      json2["hello"].erase(1, 3);
      printf("should be y but is '%s'\n", json2["hello"].string().c_str());
      dotest(json2["hello"].string() == "y");
      dotest(json2["hello"].count() == 1);
      json2["hello"].clear();
      dotest(json2["hello"].is(json::StringType));
      dotest(json2["hello"].count() == 0);
      json2.makeObject();
      dotest(json2.count() == 0);
      try {
        json::Value("{\"test\":1");
        dotest(false);
      } catch (const json::WrongType &) {
      }
      try {
        json::Value("{  ");
        dotest(false);
      } catch (const json::WrongType &) {
      }
      try {
        json::Value("{\"test\"");
        dotest(false);
      } catch (const json::WrongType &) {
      }
      try {
        json::Value("{\"test\"  ");
        dotest(false);
      } catch (const json::WrongType &) {
      }
      try {
        json::Value("{\"test\" , 5}");
        dotest(false);
      } catch (const json::WrongType &) {
      }
      try {
        json::Value("\"hello");
        dotest(false);
      } catch (const json::WrongType &) {
      }
      try {
        json::Value("[");
        dotest(false);
      } catch (const json::WrongType &) {
      }
      try {
        json::Value("{");
        dotest(false);
      } catch (const json::WrongType &) {
      }
      try {
        json::Value("{ 5");
        dotest(false);
      } catch (const json::WrongType &) {
      }
      try {
        json::Value("-");
        dotest(false);
      } catch (const json::WrongType &) {
      }
      try {
        json::Value("[ 1");
        dotest(false);
      } catch (const json::WrongType &) {
      }
      try {
        json::Value("[ 1, 2, ");
        dotest(false);
      } catch (const json::WrongType &) {
      }
      try {
        json::Value("\"\\5\"");
        dotest(false);
      } catch (const json::WrongType &) {
      }
      try {
        json::Value("test");
        dotest(false);
      } catch (const json::WrongType &) {
      }
      try {
        json::Value("nope");
        dotest(false);
      } catch (const json::WrongType &) {
      }
      try {
        json::Value("fun");
        dotest(false);
      } catch (const json::WrongType &) {
      }
      try {
        json::Value("0.0.0.0");
        dotest(false);
      } catch (const json::WrongType &) {
      }
      try {
        json::Value("0").clear();
        dotest(false);
      } catch (const json::WrongType &) {
      }
      try {
        json::Value("0").erase(0, 1);
        dotest(false);
      } catch (const json::WrongType &) {
      }
      dotest(json::Value("\"\\b\"").format() == "\"\\b\"");

      json::Value json3("[1,2,3]");

      dotest(json3.count() == 3);
      json3.makeArray();
      dotest(json3.is(json::ArrayType));
      dotest(json3.count() == 0);

      json3 = 3;
      dotest(json3.is(json::IntegerType));
      dotest(json3.integer() == 3);
      json3 = 4;
      dotest(json3.is(json::IntegerType));
      dotest(json3.integer() == 4);

      json3 = 3.14159265;
      dotest(json3.is(json::RealType));
      dotest(::fabs(json3.real() - 3.14159265) < 0.00000001);
      json3 = 2.7;
      dotest(json3.is(json::RealType));
      dotest(::fabs(json3.real() - 2.7) < 0.01);

      json3 = true;
      dotest(json3.is(json::BooleanType));
      dotest(json3.boolean());
      json3 = false;
      dotest(json3.is(json::BooleanType));
      dotest(!json3.boolean());

      json::Value test2;
      json::Value test3;
      json::Value test4;
      json::Value test5;

      test2.makeObject();
      test2["list"].makeArray();
      test2["list"].append(json::Value("\"Test \\\"me\\\"\""));
      test2["number"] = 1;
      testConstObject(test2, "number", json::Value() = 1);
      test2["boolean"] = true;
      test2["null"];
      test3.makeObject()["inner"] = test2.format();
      test3["name"] = "inner \"one\"";
      test4.makeObject()["twice"] = test3.format();
      printf("'%s'\n", test4.format().c_str());
      dotest(json::Value().parse(test4.format()).format() == test4.format());
      dotest(json::Value().parse(test4.format()) == test4);
      test5.parse(test4["twice"].string());
      dotest(test5.format() == test3.format());
      dotest(json::Value().parse(test5["inner"].string()).format() ==
             test2.format())
          dotest(json::Value().parse(test5["inner"].string()) == test2)

              printf("test2: %s\n", test2.format(4).c_str());
      printf("test3: %s\n", test3.format(4).c_str());
      printf("test4: %s\n", test4.format(4).c_str());
      printf("test5: %s\n", test5.format(4).c_str());
      printf("test6: %s\n", test6.format(4).c_str());
      printf("test7: %s\n", test7.format(4).c_str());
      printf("test8: %s\n", test8.format(4).c_str());
      printf("test9: %s\n", test9.format(4).c_str());

      const char *j3 =
          "{\"component\":\"Student_18.0.0\",\"licenseData\":\"{\\\"pn\\\":"
          "\\\"Student_PKG\\\",\\\"pv\\\":\\\"18.0.0\\\",\\\"ed\\\":"
          "\\\"Student "
          "Edition\\\",\\\"lt\\\":\\\"Main\\\",\\\"st\\\":\\\"Activated\\\","
          "\\\"ue\\\":\\\"john.doe@company.com\\\",\\\"up\\\":\\\"2685783\\\","
          "\\\"sn\\\":\\\"\\\",\\\"el\\\":\\\"\\\",\\\"ac\\\":\\\"VP6B-28NB-"
          "C83Z-RBWM-MLYD\\\",\\\"ti\\\":"
          "\\\"92FD44443BB52D4FE05400144FF9CD74\\\"}\",\"sessionData\":\"\"}";
      const std::string j4 = json::Value().parse(j3)["licenseData"].string();

      printf("-=- original -=-\n'%s'\n-=- parsed -=-\n'%s'\n", j3,
             json::Value().parse(j3).format().c_str());
      printf("-=- original -=-\n'%s'\n-=- parsed -=-\n'%s'\n", j4.c_str(),
             json::Value().parse(j4).format().c_str());

      json::Value o1(json::ObjectType);

      o1["real"] = 0.0;
      testConstObject(o1, "real", json::Value() = 0.0);
    } catch (const std::exception &exception) {
      fprintf(stderr, "FAILED: Exception: %s\n", exception.what());
    }
  }
  return 0;
}
