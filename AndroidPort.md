# Introduction #

This page explains the port for Android.

It includes:
  * References for installing Android SDK/NDK, Qt 5.2 and OpenCV
  * A few tricks for porting

# Details #

## Installing Qt for Android ##
Reference:
https://qt-project.org/doc/qt-5/androidgs.html

**=> download Qt5.2.1 online installer
http://download.qt-project.org/official_releases/online_installers/qt-opensource-linux-x64-1.5.0-2-online.run**

```
tof@tofx230:~/Downloads$ chmod 755 qt-opensource-linux-x64-1.5.0-2-online.run 
tof@tofx230:~/Downloads$ sudo ./qt-opensource-linux-x64-1.5.0-2-online.run

(extract)Note: You must set the JAVA_HOME environment variable to the JDK install directory path so that Qt Creator finds the binaries required to build your application.
```
=>
```
$ JAVA_HOME=$(readlink -f /usr/bin/java | sed "s:bin/java::")
$ echo $JAVA_HOME
```

## Installing OpenCV for Android ##

Reference : http://opencv.org/platforms/android.html
-> http://docs.opencv.org/doc/tutorials/introduction/android_binary_package/dev_with_OCV_on_Android.html

2014-04-25 : added note after moving to 2.4.9.1:
```
tof@tofx230:~/Downloads/adt-bundle-linux-x86_64-20130917/sdk/platform-tools$ ./adb install ~/src/OpenCV-2.4.9-android-sdk/apk/OpenCV_2.4.9_Manager_2.18_armv7a-neon.apk 
```

## Building the bundle for Android ##

With QtCreator, it's simple!
  * on Android 4.1, you have to send the .apk by email and install it
  * on Android 4.4, just plug the device with USB, and select Deploy or Run, and select the device. It just works!

## Qt+Android Tricks ##


### Missing icon ###

Problem: the custom icon is not displayed
Solution: edit manually the _AndroidManifest.xml_ file and check if there is "android:icon="@drawable/icon"" in the "application" section:
```
    <application android:name="org.qtproject.qt5.android.bindings.QtApplication" android:icon="@drawable/icon" android:label="@string/app_name" android:hardwareAccelerated="true">
```

### Fix the orientation on landscape or portrait mode ###

To fix the orientation to portrait mode, use the fields "screenOrientation" to change from "unspecified" to "sensorLandscape"
```
$ vi android/AndroidManifest.xml 
```
=> in landscape mode:
```
android:screenOrientation="sensorLandscape" 
```

or portrait (for the WAFmeter):
```
android:screenOrientation="sensorPortrait" 
```