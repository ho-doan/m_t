import Flutter
import UIKit
import UserNotifications
import NetworkExtension
import Combine

public class LocalPushConnectivityPlugin: NSObject, FlutterPlugin, LocalPushConnectivityPigeonHostApi {
    private let dispatchQueue = DispatchQueue(label: "PushConfigurationManager.dispatchQueue", qos: .background)

    private static var backgroundStatusCancellable: AnyCancellable?
    
    private static let settingsKey = "settings"
    private static let appStateKey = "isExecutingInBackground"
    private static let groupId = Bundle.main.object(forInfoDictionaryKey: "GroupNEAppPushLocal") as? String
    private static let userDefaults: UserDefaults = {
        if let groupId = groupId,
           let defaults = UserDefaults(suiteName: groupId) {
            defaults.set("hello", forKey: "testKey")
            defaults.synchronize()
            print("✅ App Group write success to \(groupId)")
            return defaults
        } else {
            print("⚠️ App Group không khả dụng, fallback về UserDefaults.standard")
            return UserDefaults.standard
        }
    }()
    
    private var settings: Settings!
    
    private let pushManagerDescription = "SimplePushDefaultConfiguration"
    private let pushProviderBundleIdentifier: String? = Bundle.main.object(forInfoDictionaryKey: "NEAppPushBundleId") as? String
    private var pushManager: NEAppPushManager? = nil
    
    private var flutterApi: LocalPushConnectivityPigeonFlutterApi? = nil
    
    public static func register(with registrar: FlutterPluginRegistrar) {
        let instance = LocalPushConnectivityPlugin()
        instance.settings = Self.fetchSettings()
        LocalPushConnectivityPigeonHostApiSetup.setUp(binaryMessenger: registrar.messenger(), api: instance)
        instance.flutterApi = LocalPushConnectivityPigeonFlutterApi(binaryMessenger: registrar.messenger())
        UNUserNotificationCenter.current().delegate = instance
    }
    
    private func initializePush() {
        self.dispatchQueue.async { [weak self] in
            guard let self = self else { return }
            guard let pushProviderBundleIdentifier = pushProviderBundleIdentifier else { return }
            NEAppPushManager.loadAllFromPreferences { managers, error in
                if let error = error {
                    print("Failed to load all managers from preferences: \(error)")
                    return
                }
                
                print("manager length: \(managers?.count)")
                if let manager = managers?.first {
                    manager.delegate = self
                    self.preparePush(pushManager: manager)
                } else {
                    // No existing manager. If settings look configured, create + save.
                    let newManager = NEAppPushManager()
                    self.savePush(pushManager: newManager)
                }
            }
        }
    }
    
    private func preparePush(pushManager: NEAppPushManager) {
        pushManager.loadFromPreferences { [weak self] error in
            guard let self = self else { return }
            if error == nil {
                self.pushManager = pushManager
                self.pushManager?.isEnabled = true
                self.pushManager?.delegate = self
                print("preparePush ok")
            } else {
                print("preparePush error: \(error)")
            }
        }
    }
    
    private func savePush(pushManager: NEAppPushManager) {
        self.dispatchQueue.async { [weak self] in
            guard let self = self else { return }
            pushManager.localizedDescription = self.pushManagerDescription
            pushManager.providerBundleIdentifier = self.pushProviderBundleIdentifier
            pushManager.delegate = self
            pushManager.isEnabled = true
            pushManager.providerConfiguration = [
                "host": settings.host
            ]
            if let ssid = settings.ssid {
                pushManager.matchSSIDs = ssid
            }
            pushManager.saveToPreferences { error in
                if let error = error as? NEAppPushManagerError {
                    print("NEAppPushError: \(error.localizedDescription)")
                } else if let error = error {
                    print("save push error: \(error.localizedDescription)")
                }
                print("save push ok")
            }
            self.preparePush(pushManager: pushManager)
        }
    }

    func flutterApiReady(completion: @escaping (Result<Void, Error>) -> Void){
        completion(.success(()))
    }
    
    func initialize(systemType: Int64, android: AndroidSettingsPigeon?, windows: WindowsSettingsPigeon?, ios: IosSettingsPigeon?, mode: TCPModePigeon, completion: @escaping (Result<Bool, Error>) -> Void){
        if ios == nil {
            completion(.failure(NSError(domain: "ios settings invalid", code: 1)))
            return
        }
        
        var settings = Settings()
        settings.host = mode.host
        settings.publicKey = mode.publicHasKey
        settings.port = Int(mode.port)
        settings.wsPath = mode.path
        settings.wss = mode.connectionType == .wss
        settings.useTcp = mode.connectionType == .tcpTls
        settings.deviceId = UIDevice.current.identifierForVendor?.uuidString
        settings.systemType = Int(systemType)
        if let ssids = ios?.ssids {
            settings.ssid = ssids
        }
        
        self.settings = self.settings.copyWith(settings: settings)
        try? self.set(settings: self.settings)
        
        self.initializePush()
        
        completion(.success(true))
    }

    func deviceID(completion: @escaping (Result<String, Error>) -> Void){
        completion(.success(UIDevice.current.identifierForVendor?.uuidString ?? ""))
    }

    func registerUser(user: UserPigeon, completion: @escaping (Result<Bool, Error>) -> Void){
        var settings = Settings()
        settings.connectorID = user.connectorID
        settings.connectorTag = user.connectorTag
        self.settings = self.settings.copyWith(settings: settings)
        try? self.set(settings: self.settings)
        completion(.success(true))
    }
    
    func config(mode: TCPModePigeon, ssids: [String]?, completion: @escaping (Result<Bool, Error>) -> Void){
        var settings = Settings()
        settings.host = mode.host
        settings.publicKey = mode.publicHasKey
        settings.port = Int(mode.port)
        settings.wsPath = mode.path
        settings.wss = mode.connectionType == .wss
        settings.useTcp = mode.connectionType == .tcp
        if let ssids = ssids {
            settings.ssid = ssids
        }
        
        self.settings = self.settings.copyWith(settings: settings)
        try? self.set(settings: self.settings)
        completion(.success(true))
    }
    func requestPermission(completion: @escaping (Result<Bool, Error>) -> Void){
        UNUserNotificationCenter.current().requestAuthorization(options: [.alert, .badge, .sound]){
            granted, error in
            if granted == true && error == nil {
                print("notification permission granted")
                completion(.success(true))
            } else {
                print("notification permission denied or error: \(error)")
                completion(.success(false))
            }
        }
    }
    func start(completion: @escaping (Result<Bool, Error>) -> Void){
        completion(.success(true))
    }
    func stop(completion: @escaping (Result<Bool, Error>) -> Void){
        completion(.success(true))
    }
    
    private func set(settings: Settings) throws {
        let encoder = JSONEncoder()
        let encodedSettings = try encoder.encode(settings)
        Self.userDefaults.set(encodedSettings, forKey: Self.settingsKey)
        
        // Decide save/update vs remove based on settings content
        let shouldRemove = (settings.host == nil || settings.host?.isEmpty == true) && (settings.ssid == nil || settings.ssid?.isEmpty == true)
        if shouldRemove {
            if let manager = self.pushManager {
                manager.removeFromPreferences { [weak self] error in
                    if let error = error {
                        print("removeFromPreferences error: \(error)")
                    } else {
                        print("removeFromPreferences ok")
                    }
                    self?.cleanup()
                }
            } else {
                self.cleanup()
            }
        } else {
            self.savePush(pushManager: self.pushManager ?? NEAppPushManager())
        }
    }
    
    static private func fetchSettings() -> Settings {
        guard let encodedSettings = userDefaults.data(forKey: settingsKey) else {
            return Settings()
        }
        do {
            let decoder = JSONDecoder()
            let settings = try decoder.decode(Settings.self, from: encodedSettings)
            return settings
        } catch {
            print("Error decoding settings - \(error)")
            return Settings()
        }
    }
    
    private func cleanup() {
        self.pushManager?.delegate = nil
        self.pushManager = nil
        print("cleanup done")
    }
    
    public static func initializePlugin() {
        let dispatchQueue = DispatchQueue(label: "AppDelegate.dispatchQueue")
        var isExecutingInBackgroundPublisher: AnyPublisher<Bool, Never> = {
            let initialState = UIApplication.shared.applicationState != .active

            return Just(initialState)
                .merge(with:
                    NotificationCenter.default.publisher(for: UIApplication.didEnterBackgroundNotification)
                        .map { _ in true }
                        .merge(with:
                            NotificationCenter.default.publisher(for: UIApplication.willEnterForegroundNotification)
                                .map { _ in false }
                        )
                )
                .debounce(for: .milliseconds(100), scheduler: dispatchQueue)
                .eraseToAnyPublisher()
        }()
        
        backgroundStatusCancellable = isExecutingInBackgroundPublisher.sink { status in
            Self.userDefaults.set(status, forKey: Self.appStateKey)
        }
    }
}

extension LocalPushConnectivityPlugin: NEAppPushDelegate {
    public func appPushManager(_ manager: NEAppPushManager, didReceiveIncomingCallWithUserInfo userInfo: [AnyHashable : Any] = [:]) {
        print("appPushManager isActive: \(manager.isActive)")
    }
}

extension LocalPushConnectivityPlugin: UNUserNotificationCenterDelegate {
    public func userNotificationCenter(_ center: UNUserNotificationCenter, didReceive response: UNNotificationResponse, withCompletionHandler completionHandler: @escaping () -> Void) {
        print("userNotificationCenter didReceive response: \(response.notification.request.content.userInfo["payload"])")
        if let payload = response.notification.request.content.userInfo["payload"] as? String {
            flutterApi?.onMessage(mrp: MessageSystemPigeon(fromNotification: true, mrp: MessageResponsePigeon(notification: NotificationPigeon(title: "a", body: "b"), mPayload: payload))) {
                result in
                switch result {
                case .failure(let error):
                    print("send new message background error: \(error)")
                case .success(()):
                    print("send message background ok")
                }
            }
        }
        completionHandler()
    }
    
    public func userNotificationCenter(_ center: UNUserNotificationCenter, willPresent notification: UNNotification, withCompletionHandler completionHandler: @escaping (UNNotificationPresentationOptions) -> Void) {
        print("userNotificationCenter willPresent notification: \(notification.request.content.userInfo["payload"])")
        if let payload = notification.request.content.userInfo["payload"] as? String {
            flutterApi?.onMessage(mrp: MessageSystemPigeon(fromNotification: false, mrp: MessageResponsePigeon(notification: NotificationPigeon(title: "a", body: "b"), mPayload: payload))) {
                result in
                switch result {
                case .failure(let error):
                    print("send new message in app error: \(error)")
                case .success(()):
                    print("send message in app ok")
                }
            }
        }
        completionHandler([])
    }
}
