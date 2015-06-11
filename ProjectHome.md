**WAFMeter is a WAF metering software**

Just for Fun...

It uses image processing to computer the Woman Acceptance Factor of the object in a picture:
  * colors: shiny colors (green, fuschia, ...) are better than grey
  * shapes: curves are better than angles
  * ease of use : the fewer buttons...

It's a fun software you can use on fridges, cars, room, electric tools ...

![http://wafmeter.googlecode.com/svn/wiki/images/Screenshot_android.png](http://wafmeter.googlecode.com/svn/wiki/images/Screenshot_android.png)

There are 3 dials :
  * Green dial for WAF of colors
  * Grey one for WAF of shape and simplicity
  * Black biggest dial for global result

The icons on bottoms are for the 4 modes:
  * Live webcam
  * Play movie file
  * Picture file
  * Desktop grabbing: to get the WAF of an object on a webpage, ...


---

## Downloads ##

Since GoogleCode stopped allowing upload of files, please download Android version from here:
http://cseyve.free.fr/android/


### Status ###

Licence: GNU GPL3.
Version: beta (for Proof-Of-Concept)

Portability:
  * Desktop app: Linux, MacOS X and Windows.
  * Mobile: Android (done). No support yet for Windows Phone or iOS.


### Dependencies ###

WAFmeter is written in C/C++, and uses 2 main libraries :
  * OpenCV for image import/export (file + webcam),
  * Qt4 for portable GUI (Linux, MacOS X, Windows).
  * or Qt5 for Android or Desktop (Linux, MacOS X, Windows).