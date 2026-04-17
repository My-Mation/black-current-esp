#pragma once
// =============================================================
//  web_server.h — WiFi connection + HTTP routes
//  Serves the UI and provides the browser's API surface
// =============================================================
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "config.h"

class WebServerHandler {
public:
    bool beginWiFi();
    void beginServer();
    void update();       // Call from loop()
    String getIP() const { return WiFi.localIP().toString(); }

private:
    WebServer _srv{WEB_PORT};

    // Route handlers
    void _handleRoot();
    void _handleApiState();
    void _handleApiMode();
    void _handleApiStartTest();
    void _handleApiNextQuestion();
    void _handleApiLoadQuestions();
    void _handleApiSubmit();
    void _handleCors();
    void _handleNotFound();

    // Helpers
    void _sendJson(const String& json);
    void _sendOk();
};

extern WebServerHandler gWebServer;
