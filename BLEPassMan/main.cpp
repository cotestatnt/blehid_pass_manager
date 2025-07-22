// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "connectionhandler.h"
#include "devicefinder.h"
#include "devicehandler.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QLoggingCategory>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>


using namespace Qt::StringLiterals;


int main(int argc, char *argv[])
{
    // qputenv("QT_BLUETOOTH_BACKEND", "winrt");
    qputenv("QT_QUICK_CONTROLS_MATERIAL_THEME", "Dark");   // o "Light"
    QQuickStyle::setStyle("Material");

    QGuiApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription(u"Bluetooth Low Energy Heart Rate Game"_s);
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption verboseOption(u"verbose"_s, u"Verbose mode"_s);
    parser.addOption(verboseOption);
    parser.process(app);

    ConnectionHandler connectionHandler;
    DeviceHandler deviceHandler;
    DeviceFinder deviceFinder(&deviceHandler);

    QQmlApplicationEngine engine;
    engine.setInitialProperties({
        {u"connectionHandler"_s, QVariant::fromValue(&connectionHandler)},
        {u"deviceFinder"_s, QVariant::fromValue(&deviceFinder)},
        {u"deviceHandler"_s, QVariant::fromValue(&deviceHandler)}
    });


    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,  []() { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.loadFromModule("BLEPassMan", "Main");

    return app.exec();
}
