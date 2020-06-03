#include "os/AddressIPv4.h"
#include "os/AddressIPv6.h"
#include "os/SocketServer.h"
#include "protocol/HTTP.h"
#include <stdio.h>
#include <thread>

#define dotest(condition)                                                      \
  if (!(condition)) {                                                          \
    fprintf(stderr, "FAIL(%s:%d): %s\n", __FILE__, __LINE__, #condition);      \
  }

void testConstHeaders(const http::Headers &h, const std::string &name,
                      const std::string &value) {
  dotest(h[name] == value);
}

void testHeaders(bool print) {
  const char *const header1 =
      "Host: net1.tutsplus.com\r\n"
      "User-Agent: Mozilla/5.0 (Windows; U; Windows NT 6.1; en-US; rv:1.9.1.5) "
      "\r\n"
      "\tGecko/20091102 Firefox/3.5.5 (.NET CLR 3.5.30729)\r\n"
      "Accept: "
      "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
      "Accept-Language: en-us,en;q=0.5\r\n"
      "Accept-Encoding: gzip,deflate\r\n"
      "Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.7\r\n"
      "Keep-Alive: 300\r\n"
      "Connection: keep-alive\r\n"
      "Cookie: PHPSESSID=r2t5uvjq435r4q7ib3vtdjq120\r\n"
      "Pragma: no-cache\r\n"
      "Cache-Control: no-cache\r\n"
      "\r\n"
      "Body: text\r\n";
  const char *const header2 =
      "Host: net2.tutsplus.com\n"
      "User-Agent: Mozilla/5.0 (Windows; U; Windows NT 6.1; en-US; rv:1.9.1.5) "
      "\n"
      "\tGecko/20091102 Firefox/3.5.5 (.NET CLR 3.5.30729)\n"
      "Accept: "
      "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\n"
      "Accept-Language: en-us,en;q=0.5\n"
      "Accept-Encoding: gzip,deflate\n"
      "Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.7\n"
      "Keep-Alive: 300\n"
      "Connection: keep-alive\n"
      "Cookie: PHPSESSID=r2t5uvjq435r4q7ib3vtdjq120\n"
      "Pragma: no-cache\n"
      "Cache-Control: no-cache\n"
      "\n"
      "Body: text\n";
  const char *const header3 =
      "Host: net3.tutsplus.com\r"
      "User-Agent: Mozilla/5.0 (Windows; U; Windows NT 6.1; en-US; rv:1.9.1.5) "
      "\r"
      "\tGecko/20091102 Firefox/3.5.5 (.NET CLR 3.5.30729)\r"
      "Accept: "
      "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r"
      "Accept-Language: en-us,en;q=0.5\r"
      "Accept-Encoding: gzip,deflate\r"
      "Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.7\r"
      "Keep-Alive: 300\r"
      "Connection: keep-alive\r"
      "Cookie: PHPSESSID=r2t5uvjq435r4q7ib3vtdjq120\r"
      "Pragma: no-cache\r"
      "Cache-Control: no-cache\r"
      "\r"
      "Body: text\r";
  const char *const header6 =
      "Host: net4.tutsplus.com\r"
      "User-Agent: Mozilla/5.0 (Windows; U; Windows NT 6.1; en-US; rv:1.9.1.5) "
      "\r"
      "\tGecko/20091102 Firefox/3.5.5 (.NET CLR 3.5.30729)\r"
      "Accept: "
      "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r"
      "Accept-Language: en-us,en;q=0.5\r"
      "Accept-Encoding: gzip,deflate\r"
      "Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.7\r"
      "Keep-Alive: 300";
  http::Headers h6(header6);
  http::Headers h1(header1);
  http::Headers h2(header2);
  http::Headers h3(header3);
  http::Headers h4(h1);
  http::Headers h5;
  std::string headers =
      std::string(header1) + "\n" + header2 + "\n" + header3 + "\n\n" + header6;
  http::Headers::String::size_type offset = 0;

  testConstHeaders(http::Headers(headers, offset), "Host", "net1.tutsplus.com");
  headers.erase(0, offset);
  testConstHeaders(http::Headers(headers, offset), "Body", "text");
  headers.erase(0, offset);

  testConstHeaders(http::Headers(headers, offset), "Host", "net2.tutsplus.com");
  headers.erase(0, offset);
  testConstHeaders(http::Headers(headers, offset), "Body", "text");
  headers.erase(0, offset);

  testConstHeaders(http::Headers(headers, offset), "Host", "net3.tutsplus.com");
  headers.erase(0, offset);
  testConstHeaders(http::Headers(headers, offset), "Body", "text");
  headers.erase(0, offset);

  testConstHeaders(http::Headers(headers, offset), "Host", "net4.tutsplus.com");
  headers.erase(0, offset);
  dotest(headers.size() == 0);

  dotest(!h1.empty());
  dotest(h1.has("Host"));
  dotest(h1.has("User-Agent"));
  dotest(h1.has("Accept"));
  dotest(h1.has("Accept-Language"));
  dotest(h1.has("Accept-Encoding"));
  dotest(h1.has("Accept-Charset"));
  dotest(h1.has("Keep-Alive"));
  dotest(h1.has("Connection"));
  dotest(h1.has("Cookie"));
  dotest(h1.has("Pragma"));
  dotest(h1.has("Cache-Control"));
  dotest(!h1.has("Body"));
  dotest(h1["User-Agent"].find("Gecko") > 0);
  if (print)
    printf("Header 1\n----------\n%s\n----------\n", std::string(h1).c_str());

  dotest(!h2.empty());
  dotest(h2.has("Host"));
  dotest(h2.has("User-Agent"));
  dotest(h2.has("Accept"));
  dotest(h2.has("Accept-Language"));
  dotest(h2.has("Accept-Encoding"));
  dotest(h2.has("Accept-Charset"));
  dotest(h2.has("Keep-Alive"));
  dotest(h2.has("Connection"));
  dotest(h2.has("Cookie"));
  dotest(h2.has("Pragma"));
  dotest(h2.has("Cache-Control"));
  dotest(!h2.has("Body"));
  dotest(h2["User-Agent"].find("Gecko") > 0);
  if (print)
    printf("Header 2\n----------\n%s\n----------\n", std::string(h2).c_str());

  dotest(!h3.empty());
  dotest(h3.has("Host"));
  dotest(h3.has("User-Agent"));
  dotest(h3.has("Accept"));
  dotest(h3.has("Accept-Language"));
  dotest(h3.has("Accept-Encoding"));
  dotest(h3.has("Accept-Charset"));
  dotest(h3.has("Keep-Alive"));
  dotest(h3.has("Connection"));
  dotest(h3.has("Cookie"));
  dotest(h3.has("Pragma"));
  dotest(h3.has("Cache-Control"));
  dotest(!h3.has("Body"));
  dotest(h3["User-Agent"].find("Gecko") > 0);
  if (print)
    printf("Header 3\n----------\n%s\n----------\n", std::string(h3).c_str());

  dotest(!h4.empty());
  dotest(h4.has("Host"));
  dotest(h4.has("User-Agent"));
  dotest(h4.has("Accept"));
  dotest(h4.has("Accept-Language"));
  dotest(h4.has("Accept-Encoding"));
  dotest(h4.has("Accept-Charset"));
  dotest(h4.has("Keep-Alive"));
  dotest(h4.has("Connection"));
  dotest(h4.has("Cookie"));
  dotest(h4.has("Pragma"));
  dotest(h4.has("Cache-Control"));
  dotest(!h4.has("Body"));
  dotest(h4["User-Agent"].find("Gecko") > 0);
  if (print)
    printf("Header 4\n----------\n%s\n----------\n", std::string(h4).c_str());

  dotest(h5.empty());
  h4.remove("Host");
  h4.remove("User-Agent");
  h4.remove("Accept");
  h4.remove("Accept-Language");
  h4.remove("Accept-Encoding");
  h4.remove("Accept-Charset");
  h4.remove("Keep-Alive");
  h4.remove("Connection");
  h4.remove("Cookie");
  h4.remove("Pragma");
  h4.remove("Cache-Control");
  dotest(h4.empty());

  dotest(!h6.empty());
  dotest(h6.has("Host"));
  dotest(h6.has("User-Agent"));
  dotest(h6.has("Accept"));
  dotest(h6.has("Accept-Language"));
  dotest(h6.has("Accept-Encoding"));
  dotest(h6.has("Accept-Charset"));
  dotest(h6.has("Keep-Alive"));
  dotest(h6["Keep-Alive"] == "300");

  h3.remove("Cookie");
  dotest(!h3.has("Cookie"));
  h3 = h2;
  dotest(h3.has("Cookie"));

  if (print)
    fprintf(stderr,
            "expected\n--------\n%s\n---------\nproduced\n---------\n%s\n------"
            "--\n",
            "Accept: None\r\n\r\n",
            std::string(http::Headers("Accept: None\r\n\r\n")).c_str());
  dotest(std::string(http::Headers("Accept: None\r\n\r\n")) ==
         "Accept: None\r\n\r\n");
}

void testConstQuery(const http::Query &q, const std::string &name,
                    const std::string &value) {
  dotest(q[name] == value);
}

void testQuery(bool print) {
  http::Query test1;
  http::Query test2;
  http::Query test4("key4", http::Query::IsQuery);
  std::string url5("http://server.com/action.asp");
  std::string query5("?help&test=1&test=2&time=now");
  std::string anchor5("#NoStoppingUs");
  http::Query test5(url5 + query5 + anchor5, http::Query::SearchForQuery);

  dotest(test5.getOne("test") == "1");
  dotest(test5.getOne("time") == "now");
  try {
    test5.getOne("This Key Does Not Exist");
    dotest(false);
  } catch (const std::out_of_range &exception) {
    if (print)
      printf("Expected exception: %s\n", exception.what());
  }
  dotest(test5.has("help"));
  if (print)
    printf("test5: '%s'\n", std::string(test5).c_str());
  dotest(!test5.hasValue("help"));
  dotest(query5 == std::string(test5));
  if (print)
    printf("query5 = '%s' test5 = '%s'\n", query5.c_str(),
           ((std::string)test5).c_str());
  test1["key1"] = "value1";
  test2["key2"] = "value2";
  dotest(std::string(test1) == "?key1=value1");
  dotest(std::string(test2) == "?key2=value2");
  testConstQuery(test1, "key1", "value1");
  testConstQuery(test2, "key2", "value2");
  test2 = test1;
  dotest(std::string(test2) == "?key1=value1");

  http::Query test3(test1);

  dotest(std::string(test3) == "?key1=value1");
  try {
    dotest(test1.getOne("key") != "value");
    dotest(false /* We should have thrown*/);
  } catch (const std::out_of_range &exception) {
    dotest(std::string(exception.what()) == "key");
  }
  dotest(test1.has("key1"));
  dotest(test2.has("key1"));
  dotest(test3.has("key1"));
  dotest(test1.hasValue("key1"));
  dotest(test2.hasValue("key1"));
  dotest(test3.hasValue("key1"));
  dotest(test4.has("key4"));
  dotest(!test4.hasValue("key4"));
  dotest(test1.getOne("key1") == "value1");
  dotest(test2.getOne("key1") == "value1");
  dotest(test3.getOne("key1") == "value1");
  dotest(test4.getOne("key4") == "");
  dotest(http::Query::unescape(http::Query::escape("Hello World!")) ==
         "Hello World!");
  dotest(http::Query::escape("Hello World!") != "Hello World!");
  dotest(http::Query::unescape(http::Query::escape("What %?")) == "What %?");
  dotest(http::Query::unescape("Test%") == "Test%");
  dotest(http::Query::unescape("Test%0") == "Test%0");
  dotest(http::Query::unescape("Test%F") == "Test%F");
  dotest(http::Query::unescape("Test%20") == "Test ");
  dotest(http::Query::unescape("Test%3d") == "Test=");
  dotest(http::Query::unescape("Test%3D") == "Test=");
  dotest(http::Query::unescape("Test%3Z") == "Test%3Z");
  dotest(http::Query::escape(http::Query::unescape("%FF")) == "%FF")
      dotest(http::Query::escape(http::Query::unescape("%00")) == "%00")
          dotest(std::string(http::Query("key=value", http::Query::IsQuery)) ==
                 "?key=value");
  dotest(std::string(http::Query("key=value", http::Query::SearchForQuery)) ==
         "?key=value");
  dotest(std::string(http::Query("test?key=value", http::Query::IsQuery)) ==
         "?test%3Fkey=value");
  dotest(std::string(http::Query("test?key=value",
                                 http::Query::SearchForQuery)) == "?key=value");
  dotest(std::string(http::Query("key", http::Query::IsQuery)) == "?key");
  dotest(std::string(http::Query("key", http::Query::SearchForQuery)) ==
         "?key");
  dotest(std::string(http::Query("key=value#hammerTime",
                                 http::Query::IsQuery)) == "?key=value");
  dotest(std::string(http::Query("key=value#hammerTime",
                                 http::Query::SearchForQuery)) == "?key=value");
  dotest(std::string(http::Query("test?key=value#hammerTime",
                                 http::Query::IsQuery)) == "?test%3Fkey=value");
  dotest(std::string(http::Query("test?key=value#hammerTime",
                                 http::Query::SearchForQuery)) == "?key=value");
  dotest(std::string(http::Query("key#hammerTime", http::Query::IsQuery)) ==
         "?key");
  dotest(std::string(http::Query("key#hammerTime",
                                 http::Query::SearchForQuery)) == "?key");
  test1.remove("key1");
  dotest(test1["key1"] == "");
  test1.remove("key1");
  test1.add("key1", "value");
  printf("***** key1 = %s\n", test1["key1"].c_str());
  dotest(test1["key1"] == "value");
}

void testConstRequestLine(const http::RequestLine &rl,
                          const std::string &method, const std::string &path,
                          const std::string &version,
                          const std::string &protocol) {
  dotest(rl.path() == path);
  dotest(rl.protocol() == protocol);
  dotest(rl.version() == version);
  dotest(rl.method() == method);
}

void testRequestLine() {
  testConstRequestLine(http::RequestLine("GET /docs/index.html HTTP/1.1"),
                       "GET", "/docs/index.html", "1.1", "HTTP");
  dotest(http::RequestLine("GET /docs/index.html HTTP/1.1").method() == "GET");
  dotest(http::RequestLine("GET /docs/index.html HTTP/1.1").path() ==
         "/docs/index.html");
  dotest(http::RequestLine("GET /docs/index.html HTTPS/1.1").protocol() ==
         "HTTPS");
  dotest(http::RequestLine("GET /docs/index.html HTTP/1.1").version() == "1.1");
  dotest(http::RequestLine("GET /docs/index.html?key=value HTTP/1.1")
             .query()
             .getOne("key") == "value");
  dotest(http::RequestLine("GET /docs/index.html HTTPS").protocol() == "HTTPS");

  http::RequestLine request;

  request.method() = "PUT";
  request.path() = "/index.html";
  request.protocol() = "HTTPS";
  request.version() = "1.2";
  request.query()["key"] = "value";
  dotest(std::string(request) == "PUT /index.html?key=value HTTPS/1.2");

  try {
    request.query().getOne("zathura");
    fprintf(stderr, "FAILED: We should have thrown\n");
  } catch (const std::out_of_range &) {
  } catch (const std::exception &exception) {
    fprintf(stderr, "FAILED: Wrong type of Exception: %s\n", exception.what());
  }
}

void testRequest(bool print) {
  const char *const fullRequestRaw = "PUT /index.html?key=value HTTPS/1.2\r\n"
                                     "Agent: Protocol\r\n"
                                     "\r\n";
  http::Request fullRequest(fullRequestRaw);
  http::Request requestCopy(fullRequest);

  if (print)
    printf("-=- raw -=-\n%s\n-=- processed -=-\n%s\n", fullRequestRaw,
           std::string(fullRequest).c_str());
  dotest(std::string(fullRequest) == fullRequestRaw);
  fullRequest.info().method() = "GET";
  fullRequest.fields()["Agent"] = "Testing";
  dotest(std::string(fullRequest) != fullRequestRaw);
  fullRequest = requestCopy;
  dotest(std::string(fullRequest) == fullRequestRaw);
}

void testConstResponseLine(const http::ResponseLine &rl,
                           const std::string &protocol,
                           const std::string &version, const std::string &code,
                           const std::string &message) {
  dotest(rl.protocol() == protocol);
  dotest(rl.version() == version);
  dotest(rl.code() == code);
  dotest(rl.message() == message);
}

void testResponseLine(bool print) {
  if (print)
    fprintf(stderr, "protocol='%s'\n",
            http::ResponseLine("FTP/5.1 200 OK\r\n").protocol().c_str());
  if (print)
    fprintf(stderr, "version='%s'\n",
            http::ResponseLine("FTP/5.1 200 OK\r\n").version().c_str());
  testConstResponseLine(http::ResponseLine("FTP/5.1 200 OK\r\n"), "FTP", "5.1",
                        "200", "OK");
  dotest(http::ResponseLine("FTP/5.1 200 OK\r\n").protocol() == "FTP");
  dotest(http::ResponseLine("FTP/5.1 200 OK\r\n").version() == "5.1");
  dotest(http::ResponseLine("FTP/5.1 200 OK\r\n").code() == "200");
  dotest(http::ResponseLine("FTP/5.1 200 OK\r\n").message() == "OK");
  dotest(http::ResponseLine("FTP/5.1 200 OK\r").protocol() == "FTP");
  dotest(http::ResponseLine("FTP/5.1 200 OK\r").version() == "5.1");
  dotest(http::ResponseLine("FTP/5.1 200 OK\r").code() == "200");
  dotest(http::ResponseLine("FTP/5.1 200 OK\r").message() == "OK");
  dotest(http::ResponseLine("FTP/5.1 200 OK\n").protocol() == "FTP");
  dotest(http::ResponseLine("FTP/5.1 200 OK\n").version() == "5.1");
  dotest(http::ResponseLine("FTP/5.1 200 OK\n").code() == "200");
  dotest(http::ResponseLine("FTP/5.1 200 OK\n").message() == "OK");
  dotest(http::ResponseLine("FTP/5.1 200 OK").protocol() == "FTP");
  dotest(http::ResponseLine("FTP/5.1 200 OK").version() == "5.1");
  dotest(http::ResponseLine("FTP/5.1 200 OK").code() == "200");
  dotest(http::ResponseLine("FTP/5.1 200 OK").message() == "OK");
  dotest(http::ResponseLine("FTP").protocol() == "FTP");
  dotest(http::ResponseLine().protocol() == "HTTP");
  if (print)
    printf("empty response line protocol = '%s'\n",
           http::ResponseLine().protocol().c_str());

  http::ResponseLine response;

  response.protocol() = "HTTPS";
  response.version() = "3.1";
  response.code() = "301";
  response.message() = "Moved Permanently";
  dotest(std::string(response) == "HTTPS/3.1 301 Moved Permanently");
}

void testResponse(bool print) {
  const char *const fullResponseRaw = "HTTPS/1.9 200 OK\r\n"
                                      "Agent: Protocol\r\n"
                                      "\r\n";
  http::Response fullResponse(fullResponseRaw);
  http::Response responseCopy(fullResponse);

  if (print)
    printf("-=- raw response -=-\n%s\n-=- processed response -=-\n%s\n",
           fullResponseRaw, std::string(fullResponse).c_str());
  if (print)
    printf("-=- processed line -=-\n%s\n",
           std::string(fullResponse.info()).c_str());
  if (print)
    printf("-=- processed line message -=-\n%s\n",
           std::string(fullResponse.info().message()).c_str());
  dotest(std::string(fullResponse) == fullResponseRaw);
  fullResponse.info().protocol() = "FTP";
  fullResponse.fields()["Agent"] = "Testing";
  dotest(std::string(fullResponse) != fullResponseRaw);
  fullResponse = responseCopy;
  dotest(std::string(fullResponse) == fullResponseRaw);
}

static bool ServerThreadRunning = true;

std::string readline(net::Socket *connection) {
  std::string buffer;
  std::string line = "";

  while (buffer != "\n") {
    buffer = connection->read(1);
    line += buffer;
  }
  return line;
}

void serverThread(int port) {
  net::AddressIPv4 serverAddress(port);
  net::SocketServer server(serverAddress.family());
  printf("Listening at http://localhost:%d/\n", port);
  printf("To quit, go to http://localhost:%d/quit\n", port);
  server.reuseAddress();
  server.reusePort();
  server.bind(serverAddress);
  server.listen(10);
  std::string line, buffer;

  try {
    while (ServerThreadRunning) {
      net::AddressIPv6 connectedTo;
      net::Socket *connection = new net::Socket();

      printf("THREAD: %zx: Waiting for connection\n",
             std::hash<std::thread::id>{}(std::this_thread::get_id()));
      server.accept(connectedTo, *connection);
      printf("THREAD: %zx: Connection received\n",
             std::hash<std::thread::id>{}(std::this_thread::get_id()));
      buffer.clear();
      do {
        line = readline(connection);
        buffer += line;
      } while ((line != "\r\n") && (line != "\r") && (line != "\n"));

      http::Request request(buffer);
      http::Response response;

      response.info().code() = "200";
      response.info().message() = "OK";

      buffer = response;
      connection->write(buffer);
      buffer = request;
      connection->write(buffer);
      connection->close();
      delete connection;
      if (request.info().path() == "/quit") {
        ServerThreadRunning = false;
        connection->close();
      }
      printf("THREAD %zx: Request\n%s\nTHREAD %zx: Response\n%s\n",
             std::hash<std::thread::id>{}(std::this_thread::get_id()),
             std::string(request).c_str(),
             std::hash<std::thread::id>{}(std::this_thread::get_id()),
             std::string(response).c_str());
    }
  } catch (const std::exception &exception) {
    if (!ServerThreadRunning) {
      printf("THREAD: %zx: EXPECTED: Server Thread: %s\n",
             std::hash<std::thread::id>{}(std::this_thread::get_id()),
             exception.what());
    } else {
      printf("THREAD: %zx: FAILED: Server Thread Exception: %s\n",
             std::hash<std::thread::id>{}(std::this_thread::get_id()),
             exception.what());
    }
  }
}

int main(int argc, char * /*argv*/[]) {
  int iterations = 4000;
#ifdef __Tracer_h__
  iterations = 1;
#endif
  if (argc == 2) {
    std::thread server(serverThread, 8123);

    server.join();
  }
  for (int i = 0; i < iterations; ++i) {
    try {
      testHeaders(0 == i);
      testQuery(0 == i);
      testRequestLine();
      testRequest(0 == i);
      testResponseLine(0 == i);
      testResponse(0 == i);
    } catch (const std::exception &exception) {
      fprintf(stderr, "FAILED: Exception: %s\n", exception.what());
    }
  }
  return 0;
}
