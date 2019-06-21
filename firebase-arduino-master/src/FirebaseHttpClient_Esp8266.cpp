
#include "FirebaseHttpClient.h"

#include <stdio.h>

// The ordering of these includes matters greatly.
#include <WiFiClientSecure.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>

// Detect whether stable version of HTTP library is installed instead of
// master branch and patch in missing status and methods.
#ifndef HTTP_CODE_TEMPORARY_REDIRECT
#define HTTP_CODE_TEMPORARY_REDIRECT 307
#define USE_ESP_ARDUINO_CORE_2_0_0
#endif

// Firebase now returns `Connection: close` after REST streaming redirection.
//
// Override the built-in ESP8266HTTPClient to *not* close the
// connection if forceReuse it set to `true`.
class ForceReuseHTTPClient : public HTTPClient {
  public:
    void end() {
      if (_forceReuse) {
        _canReuse = true;
      }
      HTTPClient::end();
    }
    void forceReuse(bool forceReuse) {
      _forceReuse = forceReuse;
    }
  protected:
    bool _forceReuse = false;
};

class FirebaseHttpClientEsp8266 : public FirebaseHttpClient {
  public:
    String fingerprint;
    FirebaseHttpClientEsp8266() {
      int httpCode;
      Serial.println("Atualizacao do fingerprint via servidor http em leo.json");
      HTTPClient https;
      https.begin("http://fingerprint.pacelly.com/leo.json");
      httpCode = https.GET();
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        fingerprint = https.getString();
        //Serial.print("leeeo:");
        //Serial.println(fingerprint);
        fingerprint.remove(0, 1);
        fingerprint.remove(fingerprint.length(), 1);
        Serial.print("Nova fingerprint: ");
        Serial.println(fingerprint);
      }
      https.end();
    }

    void setReuseConnection(bool reuse) override {
      http_.setReuse(reuse);
      http_.forceReuse(reuse);
    }

    void begin(const std::string& url) override {
      http_.begin(url.c_str(), fingerprint);
    }

    void begin(const std::string& host, const std::string& path) override {
      http_.begin(host.c_str(), kFirebasePort, path.c_str(), fingerprint);
    }

    void end() override {
      http_.end();
    }

    void addHeader(const std::string& name, const std::string& value) override {
      http_.addHeader(name.c_str(), value.c_str());
    }

    void collectHeaders(const char* header_keys[], const int count) override {
      http_.collectHeaders(header_keys, count);
    }

    std::string header(const std::string& name) override {
      return http_.header(name.c_str()).c_str();
    }

    int sendRequest(const std::string& method, const std::string& data) override {
      return http_.sendRequest(method.c_str(), (uint8_t*)data.c_str(), data.length());
    }

    std::string getString() override {
      return http_.getString().c_str();
    }

    Stream* getStreamPtr() override {
      return http_.getStreamPtr();
    }

    std::string errorToString(int error_code) override {
      return HTTPClient::errorToString(error_code).c_str();
    }

    bool connected() override {
      return http_.connected();
    }

  private:
    ForceReuseHTTPClient http_;
};

FirebaseHttpClient* FirebaseHttpClient::create() {
  return new FirebaseHttpClientEsp8266();
}

