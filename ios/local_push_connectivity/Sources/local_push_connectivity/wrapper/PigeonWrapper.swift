struct Settings: Codable, Equatable {
    var ssid: [String]? = nil
    var host: String? = nil
    var deviceId: String? = nil
    var connectorID: String? = nil
    var connectorTag: String? = nil
    var systemType: Int? = nil
    var port: Int? = nil
    var wss: Bool? = nil
    var wsPath: String? = nil
    var useTcp: Bool? = nil
    var publicKey: String? = nil
    
    init() {}
    
    static func fromPigeon(settings: PluginSettingsPigeon) -> Settings {
        var setting = Settings()
        setting.connectorID = settings.connectorID
        setting.host = settings.host
        setting.deviceId = settings.deviceId
        setting.systemType = Int(settings.systemType ?? -1)
        setting.port = Int(settings.port ?? -1)
        setting.wss = settings.wss ?? false
        setting.wsPath = settings.wsPath
        setting.useTcp = settings.useTcp ?? false
        setting.publicKey = settings.publicKey
        setting.connectorTag = settings.connectorTag
        return setting
    }
    
    func copyWith(settings: Settings, useOldUser: Bool = true) -> Settings {
        var setting = Settings()
        setting.ssid = settings.ssid ?? self.ssid
        setting.connectorID = settings.connectorID
        if useOldUser && setting.connectorID == nil {
            setting.connectorID = self.connectorID
        }
        setting.host = settings.host ?? self.host
        setting.deviceId = settings.deviceId ?? self.deviceId
        setting.systemType = settings.systemType ?? self.systemType
        setting.port = settings.port ?? self.port
        setting.wss = settings.wss ?? self.wss
        setting.wsPath = settings.wsPath ?? self.wsPath
        setting.useTcp = settings.useTcp ?? self.useTcp
        setting.publicKey = settings.publicKey ?? self.publicKey
        setting.connectorTag = settings.connectorTag ?? self.connectorTag
        return setting
    }
}
