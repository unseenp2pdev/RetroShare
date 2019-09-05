!include("../../retroshare.pri"): error("Could not include file ../../retroshare.pri")

TARGET = retroshare-service

QT += core
QT -= gui
QT += network xml
CONFIG += console

QMAKE_CXXFLAGS *= -std=gnu++11 -D_REENTRANT -Wall -W -fPIC

INCLUDEPATH += TorControl
#DEPENDPATH  += ../../libretroshare/src/lib
#LIBS  += ../../libretroshare/src/lib

!include("../../libretroshare/src/use_libretroshare.pri"):error("Including")

android-* {
    QT += androidextras

    ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android

    DISTFILES += android/AndroidManifest.xml \
        android/res/drawable/retroshare_128x128.png \
        android/res/drawable/retroshare_retroshare_48x48.png \
        android/gradle/wrapper/gradle-wrapper.jar \
        android/gradlew \
        android/res/values/libs.xml \
        android/build.gradle \
        android/gradle/wrapper/gradle-wrapper.properties \
        android/gradlew.bat
}


appimage {
    icon_files.path = "$${PREFIX}/share/icons/hicolor/scalable/"
    icon_files.files = ../data/retroshare-service.svg
    INSTALLS += icon_files

    desktop_files.path = "$${PREFIX}/share/applications"
    desktop_files.files = ../data/retroshare-service.desktop
    INSTALLS += desktop_files
}

unix {
    target.path = "$${RS_BIN_DIR}"
    INSTALLS += target
}

# Tor controller

HEADERS += 	TorControl/AddOnionCommand.h \
                TorControl/AuthenticateCommand.h \
                TorControl/CryptoKey.h \
                TorControl/GetConfCommand.h \
                TorControl/HiddenService.h \
                TorControl/PendingOperation.h  \
                TorControl/ProtocolInfoCommand.h \
                TorControl/SecureRNG.h \
                TorControl/SetConfCommand.h \
                TorControl/Settings.h \
                TorControl/StrUtil.h \
                TorControl/TorControl.h \
                TorControl/TorControlCommand.h \
		TorControl/TorControlConsole.h \
                TorControl/TorControlSocket.h \
                TorControl/TorManager.h \
                TorControl/TorProcess.h \
                TorControl/TorProcess_p.h \
                TorControl/TorSocket.h \
                TorControl/Useful.h

SOURCES += 	TorControl/AddOnionCommand.cpp \
                                TorControl/AuthenticateCommand.cpp \
                                TorControl/GetConfCommand.cpp \
                                TorControl/HiddenService.cpp \
                                TorControl/ProtocolInfoCommand.cpp \
                                TorControl/SetConfCommand.cpp \
                                TorControl/TorControlCommand.cpp \
                                TorControl/TorControl.cpp \
				TorControl/TorControlConsole.cpp \
                                TorControl/TorControlSocket.cpp \
                                TorControl/TorManager.cpp \
                                TorControl/TorProcess.cpp \
                                TorControl/TorSocket.cpp \
                                TorControl/CryptoKey.cpp         \
                                TorControl/PendingOperation.cpp  \
                                TorControl/SecureRNG.cpp         \
                                TorControl/Settings.cpp          \
                                TorControl/StrUtil.cpp

SOURCES += retroshare-service.cc
