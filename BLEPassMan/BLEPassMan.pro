TEMPLATE = app
TARGET = ble-passman

QT += qml quick bluetooth multimedia quickcontrols2

CONFIG += qmltypes
CONFIG += c++17

RC_ICONS = favicon.ico

QML_IMPORT_NAME = BLEPassMan
QML_IMPORT_MAJOR_VERSION = 1
QML_IMPORT_PATH = $$OUT_PWD


HEADERS += \
    connectionhandler.h \
    deviceinfo.h \
    devicefinder.h \
    devicehandler.h \
    bluetoothbaseclass.h

SOURCES += main.cpp \
    connectionhandler.cpp \
    deviceinfo.cpp \
    devicefinder.cpp \
    devicehandler.cpp \
    bluetoothbaseclass.cpp

qml_resources.files = \
    qmldir \
    Main.qml \
    App.qml \
    BluetoothAlarmDialog.qml \
    Connect.qml \
    CustomButton.qml \
    ButtonsFingerprint.qml \
    Page.qml \
    Settings.qml \
    Measure.qml \
    SplashScreen.qml \
    TitleBar.qml \

qml_resources.prefix = /qt/qml/BLEPassMan

RESOURCES = qml_resources
RESOURCES += userlist/userlist.qrc
RESOURCES += images/images.qrc

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
ANDROID_BUILD_TYPE = debug


ios: QMAKE_INFO_PLIST = shared/Info.qmake.ios.plist
macos: QMAKE_INFO_PLIST = shared/Info.qmake.macos.plist

DISTFILES += \
    android/AndroidManifest.xml

