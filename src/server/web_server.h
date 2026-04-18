#pragma once
// =============================================================
//  web_server.h — WiFi connection + HTTP routes
//  Serves the UI and provides the browser's API surface
// =============================================================
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include "config.h"

class WebServerHandler {
public:
    bool beginWiFi();
    void beginServer();
    void update();       // Call from loop()
    String getIP() const { return WiFi.localIP().toString(); }

    void sendResultsToServer(); // Push final data to backend

    // Status tracking for UI
    String getFetchStatus() const { return "Push-Only"; }

private:
    WebServer _srv{WEB_PORT};
    Preferences _prefs;
    String _lastJson = "";
    bool _isFetching = false;

    // Route handlers
    void _handleRoot();
    void _handleApiState();
    void _handleApiMode();
    void _handleApiStartTest();
    void _handleApiNextQuestion();
    void _handleApiLoadQuestions();
    void _handleApiGetQuestions();
    void _handleApiSubmit();
    void _handleApiSyncAns();
    void _handleApiResetVoice();
    void _handleApiUploadAudio();
    void _handleCors();
    void _handleNotFound();

    // Helpers
    void _sendJson(const String& json);
    void _sendOk();
    bool _parseQuestionsJson(const String& body);
};

extern WebServerHandler gWebServer;
