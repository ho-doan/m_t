package com.hodoan.local_push_connectivity

import android.Manifest
import android.annotation.SuppressLint
import android.app.Activity
import android.app.ActivityManager
import android.app.Application
import android.content.Context
import android.content.Intent
import android.content.SharedPreferences
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.provider.Settings
import android.util.Log
import androidx.core.app.ActivityCompat
import com.hodoan.local_push_connectivity.services.PushNotificationService
import com.hodoan.local_push_connectivity.wrapper.PigeonWrapper
import com.hodoan.local_push_connectivity.wrapper.PluginSettings
import io.flutter.embedding.engine.plugins.FlutterPlugin
import io.flutter.embedding.engine.plugins.activity.ActivityAware
import io.flutter.embedding.engine.plugins.activity.ActivityPluginBinding
import io.flutter.plugin.common.BinaryMessenger
import io.flutter.plugin.common.PluginRegistry

class LocalPushConnectivityPlugin : LocalPushConnectivityPigeonHostApi, FlutterPlugin,
    PluginRegistry.RequestPermissionsResultListener, PluginRegistry.NewIntentListener,
    ActivityAware {
    private lateinit var pref: SharedPreferences
    private lateinit var context: Context
    private var activity: Activity? = null

    private lateinit var binaryMessenger: BinaryMessenger

    private var callback: ((Result<Boolean>) -> Unit)? = null

    private lateinit var settings: PluginSettings

    override fun onAttachedToEngine(binding: FlutterPlugin.FlutterPluginBinding) {
        binaryMessenger = binding.binaryMessenger
        flutterApi = LocalPushConnectivityPigeonFlutterApi(binaryMessenger)
        context = binding.applicationContext
        pref = context.getSharedPreferences(PushNotificationService.PREF_NAME, Context.MODE_PRIVATE)
        val settingStr = pref.getString(PushNotificationService.SETTINGS_PREF, "")
        if (!settingStr.isNullOrEmpty()) {
            settings = PigeonWrapper.fromString(settingStr)
        }
        LocalPushConnectivityPigeonHostApi.setUp(binaryMessenger, this)
    }

    override fun initialize(
        systemType: Long,
        android: AndroidSettingsPigeon?,
        windows: WindowsSettingsPigeon?,
        ios: IosSettingsPigeon?,
        mode: TCPModePigeon,
        callback: (Result<Boolean>) -> Unit
    ) {
        if (android == null) {
            callback(
                Result.failure(
                    FlutterError("1", "android settings invalid")
                )
            )
        }
        settings = PluginSettings(
            iconNotification = android?.icon,
            channelNotification = android?.channelNotification,
            host = mode.host,
            publicKey = mode.publicHasKey,
            port = mode.port,
            wsPath = mode.path,
            wss = mode.connectionType == ConnectionType.WSS,
            useTcp = mode.connectionType == ConnectionType.TCP,
            systemType = systemType
        )
        start(callback)
    }

    override fun flutterApiReady(callback: (Result<Unit>) -> Unit) {
        callback(Result.success(Unit))
    }

    override fun config(
        mode: TCPModePigeon,
        ssids: List<String>?,
        callback: (Result<Boolean>) -> Unit
    ) {
        val newSettings = PluginSettings(
            host = mode.host,
            publicKey = mode.publicHasKey,
            port = mode.port,
            wsPath = mode.path,
            wss = mode.connectionType == ConnectionType.WSS,
            useTcp = mode.connectionType == ConnectionType.TCP,
        )
        settings = settings.copyWith(newSettings)
        start(callback)
    }

    override fun registerUser(user: UserPigeon, callback: (Result<Boolean>) -> Unit) {
        val newSettings = PluginSettings(
            connectorID = user.connectorID,
            connectorTag = user.connectorTag
        )
        settings = settings.copyWith(newSettings)
        start(callback)
    }

    @SuppressLint("HardwareIds")
    override fun deviceID(callback: (Result<String>) -> Unit) {
        activity?.let {
            val deviceId = Settings.Secure.getString(
                it.contentResolver, Settings.Secure.ANDROID_ID
            )
            callback(Result.success(deviceId))
            return
        }
        callback(Result.success(""))
    }

    override fun requestPermission(callback: (Result<Boolean>) -> Unit) {
        val pers = arrayListOf<Boolean>()
        for (i in permissions().toTypedArray()) {
            pers +=
                ActivityCompat.checkSelfPermission(
                    context,
                    i
                ) == PackageManager.PERMISSION_GRANTED
        }
        val permissionLst: Array<Boolean> = pers.toTypedArray()

        if (!permissionLst.any { !it }) {
            callback(Result.success(true))
            return
        }

        activity!!.requestPermissions(
            permissions().toTypedArray(), 111
        )

        this.callback = callback
    }

    @SuppressLint("CommitPrefEdits")
    override fun start(callback: (Result<Boolean>) -> Unit) {
        val settingStr = PigeonWrapper.toString(settings)
        pref.edit().apply() {
            putString(PushNotificationService.SETTINGS_PREF, settingStr)
        }
        if (activity == null) {
            callback(Result.failure(FlutterError("2", "activity is null")))
            return
        }

        if (isServiceRunning(activity!!, PushNotificationService::class.java)) {
            val intent = Intent(PushNotificationService.CHANGE_SETTING)
            intent.putExtra(PushNotificationService.SETTINGS_EXTRA, settingStr)
            activity!!.sendBroadcast(intent)
            callback(Result.success(true))
            return
        }

        val intent = Intent(activity, PushNotificationService::class.java)
        intent.putExtra(PushNotificationService.SETTINGS_EXTRA, settingStr)

        try {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                activity!!.startForegroundService(intent)
            } else {
                activity!!.startService(intent)
            }
            callback(Result.success(true))
        } catch (e: Exception) {
            callback(Result.failure(FlutterError("3", "start service error", e.message)))
        }
    }

    override fun stop(callback: (Result<Boolean>) -> Unit) {
        try {
            activity?.let {
                if (isServiceRunning(it, PushNotificationService::class.java)) {
                    settings.connectorID = null
                    start(callback)
                }
                return
            }
        } catch (e: Exception) {
            Log.e(LocalPushConnectivityPlugin::class.simpleName, "stopService: $e")
            callback(Result.failure(FlutterError("3", "stop service error", e.localizedMessage)))
        }
    }

    override fun onDetachedFromEngine(binding: FlutterPlugin.FlutterPluginBinding) {
        flutterApi = null
    }

    override fun onAttachedToActivity(binding: ActivityPluginBinding) {
        activity = binding.activity
        binding.addRequestPermissionsResultListener(this)
        binding.addOnNewIntentListener(this)

        activity?.application?.registerActivityLifecycleCallbacks(object :
            Application.ActivityLifecycleCallbacks {
            override fun onActivityCreated(p0: Activity, p1: Bundle?) {}

            override fun onActivityDestroyed(p0: Activity) {}

            override fun onActivityPaused(p0: Activity) {
                appOpen = false
            }

            override fun onActivityResumed(p0: Activity) {
                appOpen = true
            }

            override fun onActivitySaveInstanceState(p0: Activity, p1: Bundle) {}

            override fun onActivityStarted(p0: Activity) {}

            override fun onActivityStopped(p0: Activity) {
                appOpen = false
            }
        })
    }

    override fun onDetachedFromActivityForConfigChanges() {}

    override fun onReattachedToActivityForConfigChanges(binding: ActivityPluginBinding) {
        binding.removeRequestPermissionsResultListener(this)
        binding.removeOnNewIntentListener(this)
    }

    override fun onDetachedFromActivity() {
        activity = null
    }

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray
    ): Boolean {
        val r = requestCode == 111 && !grantResults.toList()
            .any { it != PackageManager.PERMISSION_GRANTED }
        callback?.let { it(Result.success(r)) }
        callback = null
        return r
    }

    override fun onNewIntent(intent: Intent): Boolean {
        flutterApi = LocalPushConnectivityPigeonFlutterApi(binaryMessenger)
        val newMessage = intent.getStringExtra("payload")
        newMessage?.let {
            Log.d(TAG, "onNewIntent: $newMessage $flutterApi")
            val m = MessageResponsePigeon(
                NotificationPigeon("a", "a"), it
            )
            flutterApi?.onMessage(
                MessageSystemPigeon(true,m)
            ) { result ->
                Log.d(TAG, "onNewIntent: ${result.isSuccess}")
            }
        }
        return true
    }

    companion object {
        val TAG = LocalPushConnectivityPlugin::class.simpleName

        var flutterApi: LocalPushConnectivityPigeonFlutterApi? = null
        var appOpen = true;

        fun isServiceRunning(activity: Activity, serviceClass: Class<*>): Boolean {
            val activityManager =
                activity.getSystemService(Context.ACTIVITY_SERVICE) as ActivityManager
            Log.e(
                LocalPushConnectivityPlugin::class.simpleName,
                "isServiceRunning: ${
                    activityManager.getRunningServices(Int.MAX_VALUE).map { it.service.className }
                } ${serviceClass.name}",
            )
            for (serviceInfo in activityManager.getRunningServices(Int.MAX_VALUE)) {
                if (serviceClass.name == serviceInfo.service.className) {
                    return true // Service is running
                }
            }
            return false // Service is not running
        }

        private fun permissions(): ArrayList<String> {
            val permissionLst = arrayListOf(
                Manifest.permission.ACCESS_COARSE_LOCATION,
                Manifest.permission.ACCESS_FINE_LOCATION,
            )

            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.UPSIDE_DOWN_CAKE) {
                permissionLst += Manifest.permission.FOREGROUND_SERVICE_LOCATION
            }
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                permissionLst += Manifest.permission.POST_NOTIFICATIONS
            }

            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
                permissionLst += Manifest.permission.FOREGROUND_SERVICE
            }
            Log.d(TAG, "permissions: $permissionLst")
            return permissionLst
        }
    }
}
