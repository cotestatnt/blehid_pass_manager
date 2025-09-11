#ifndef PLACEHOLDERPARSER_H
#define PLACEHOLDERPARSER_H

#include <QString>
#include <QByteArray>
#include <QVector>
#include <QHash>

struct KeyAction {
    quint8 modifiers = 0;
    quint8 keycode = 0;
    int delayMs = 0;     // >0 => delay dopo questo evento
    bool isRaw = false;
};

class PlaceholderParser
{
public:
    // Restituisce lista di azioni oppure riempie errorMsg e ritorna lista vuota
    static QVector<KeyAction> parse(const QString &input, QString *errorMsg = nullptr);

    // Converte KeyAction -> payload BLE secondo protocollo (adatta al tuo firmware)
    static QByteArray buildBlePayload(const QVector<KeyAction>& actions);

private:
    static bool parsePlaceholder(const QString &token, KeyAction &out, QString *errorMsg);
    static bool parseCombo(const QString &token, KeyAction &out, QString *errorMsg);
    static bool mapSingleKey(const QString &nameUpper, quint8 &keycode, bool &isModifier);
    static void appendText(const QString &text, QVector<KeyAction>& actions, QString *errorMsg);
    static quint8 charToKeycode(QChar c, quint8 &modifiers, bool &ok);

    static const QHash<QString, quint8> s_keyMap;      // tasti base
    static const QHash<QString, quint8> s_modMap;      // modificatori
};

#endif // PLACEHOLDERPARSER_H
