#pragma once
#include "nlohmann/json.hpp"
#include <string>
#include <sstream>
#include "utils.h"

using json = nlohmann::json;

struct PingModel {
    std::string messageType;
};

inline void to_json(json& j, const PingModel& s) {
    j = json{
        {"messageType",s.messageType},
    };
}

inline void from_json(const json& j, PingModel& s) {
    j.at("messageType").get_to(s.messageType);
}

struct PongModel {
    std::optional<std::string> pong;
};

inline void to_json(json& j, const PongModel& s) {
    j = json{};
    if (s.pong) j["pong"] = *s.pong;
}

inline void from_json(const json& j, PongModel& p) {
    if (j.contains("pong")) p.pong = j.at("pong").get<std::string>();
}
// ====================== NotificationResponse ======================
struct NotificationResponse {
    std::string title;
    std::string body;
    std::optional<std::string> subtitle;
};

inline void to_json(json& j, const NotificationResponse& s) {
    j = json{
        {"Title", s.title},
        {"Body", s.body},
    };
    if (s.subtitle)j["Subtitle"] = *s.subtitle;
}

inline void from_json(const json& j, NotificationResponse& p) {
    j.at("Title").get_to(p.title);
    j.at("Body").get_to(p.body);
    if (j.contains("Subtitle")) p.subtitle = j.at("Subtitle").get<std::string>();
}
// ====================== NotificationResponse ======================

struct MessageResponse {
    std::optional<NotificationResponse> notification;
};
//// ========= MessageResponse ======
inline void to_json(json& j, const MessageResponse& s) {
    j = json{};
    if (s.notification) j["Notification"] = *s.notification;
}

inline void from_json(const json& j, MessageResponse& p) {
    if (j.contains("Notification")) p.notification = j.at("Notification").get<NotificationResponse>();
}
// ========= MessageResponse ======


struct Sender {
    std::string connectorID;
    std::string connectorTag;
    std::string deviceID;
};

struct DataRegister {
    std::optional<std::string> apnsToken;
    std::optional<std::string> applicationID;
    std::optional<int> apnsServerType;
};

struct RegisterModel {
    std::string messageType;
    int systemType;
    Sender sender;
    DataRegister data;

};


// ================== sender ================================
inline void to_json(json& j, const Sender& s) {
    j = json{
        {"connectorID",s.connectorID},
        {"connectorTag",s.connectorTag},
        {"deviceID",s.deviceID},
    };
}

inline void from_json(const json& j, Sender& s) {
    j.at("connectorID").get_to(s.connectorID);
    j.at("connectorTag").get_to(s.connectorTag);
    j.at("deviceID").get_to(s.deviceID);
}
// ================== sender ================================
// ================== data ================================
inline void to_json(json& j, const DataRegister& s) {
    j = json{};
    if (s.apnsToken)j["apnsToken"] = *s.apnsToken;
    if (s.applicationID)j["applicationID"] = *s.applicationID;
    if (s.apnsServerType)j["apnsServerType"] = *s.apnsServerType;
}

inline void from_json(const json& j, DataRegister& s) {
    if (j.contains("apnsToken"))
        s.apnsToken = j.at("apnsToken").get<std::string>();
    if (j.contains("applicationID"))
        s.applicationID = j.at("applicationID").get<std::string>();
    if (j.contains("apnsServerType"))
        s.apnsServerType = j.at("apnsServerType").get<int>();
}
// ================== data ================================
// ================== register ================================
inline void to_json(json& j, const RegisterModel& s) {
    j = json{
        {"messageType",s.messageType},
        {"systemType",s.systemType},
        {"sender",s.sender},
        {"data",s.data},
    };
}

inline void from_json(const json& j, RegisterModel& s) {
    j.at("messageType").get_to(s.messageType);
    j.at("systemType").get_to(s.systemType);
    j.at("sender").get_to(s.sender);
    j.at("data").get_to(s.data);
}
// ================== register ================================


struct PluginSetting {
    long long appPid;
    std::string title;
    std::string title_notification;
    std::string appBundle;
    std::string displayName;
    std::string iconPath;
    std::string iconContent;
    std::string host;
    std::int64_t port;
    std::int64_t systemType;
    std::string publicHasKey;
    bool wss;
    std::string path;
    bool tcp;
    std::string connector_tag;
    std::string connector_id;

    PluginSetting() = default;

    std::wstring uri() const {
        wchar_t _uri[256];
        std::wstring m = wss ? L"wss" : L"ws";
        swprintf(_uri, 256, L"%ls://%ls:%lld%ls",
            m.c_str(),
            utf8_to_wide(host).c_str(),
            port,
            utf8_to_wide(path).c_str()
        );

        std::wstring uk(_uri);
        return uk;
    }

    std::wstring registerStr() const {
        if (connector_id.empty() || connector_tag.empty())return L"";
        RegisterModel registerModel{
                    "register",
                    static_cast<int>(systemType),
                    Sender{
                        connector_id,
                        connector_tag,
                        wide_to_utf8(get_sys_device_id())
                    },
                    DataRegister{}
        };

        json j = registerModel;
        return utf8_to_wide(j.dump());
    }
};

// ================== plugin settings ================================
inline void to_json(json& j, const PluginSetting& s) {
    j = json{
        {"appPid", s.appPid},
        {"title", s.title},
        {"title_notification", s.title_notification},
        {"appBundle", s.appBundle},
        {"displayName", s.displayName},
        {"iconPath",s.iconPath},
        {"iconContent",s.iconContent},
        {"host",s.host},
        {"port", s.port},
        {"systemType", s.systemType},
        {"publicHasKey", s.publicHasKey},
        {"wss",s.wss},
        {"path", s.path},
        {"tcp", s.tcp},
        {"connector_tag",s.connector_tag},
        {"connector_id",s.connector_id},
    };
}

inline void from_json(const json& j, PluginSetting& p) {
    j.at("appPid").get_to(p.appPid);
    j.at("title").get_to(p.title);
    j.at("title_notification").get_to(p.title_notification);
    j.at("appBundle").get_to(p.appBundle);
    j.at("displayName").get_to(p.displayName);
    j.at("iconPath").get_to(p.iconPath);
    j.at("iconContent").get_to(p.iconContent);
    j.at("host").get_to(p.host);
    j.at("port").get_to(p.port);
    j.at("systemType").get_to(p.systemType);
    j.at("publicHasKey").get_to(p.publicHasKey);
    j.at("wss").get_to(p.wss);
    j.at("path").get_to(p.path);
    j.at("tcp").get_to(p.tcp);
    j.at("connector_tag").get_to(p.connector_tag);
    j.at("connector_id").get_to(p.connector_id);
}
// ================== plugin settings ================================

inline std::string pluginSettingsToJson(PluginSetting settings) {
    json j = settings;

    auto k = j.dump();
    return base64_encode(k);
}
inline std::string pluginSettingsToJson2(PluginSetting settings) {
    json j = settings;

    return j.dump();
}
inline PluginSetting pluginSettingsFromJson(std::string json_str) {
    std::string decode = base64_decode(json_str);
    json j = json::parse(decode);
    PluginSetting p = j.get<PluginSetting>();
    return p;
}