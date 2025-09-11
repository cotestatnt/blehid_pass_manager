#include "placeholderparser.h"
#include <QRegularExpression>

const QHash<QString, quint8> PlaceholderParser::s_modMap = {
    { "CTRL",  0x01 },
    { "SHIFT", 0x02 },
    { "ALT",   0x04 },
    { "GUI",   0x08 },
    { "WIN",   0x08 }  // alias
};

const QHash<QString, quint8> PlaceholderParser::s_keyMap = {
    { "ENTER", 0x28 },
    { "TAB",   0x2B },
    { "ESC",   0x29 },
    { "ESCAPE",0x29 },
    { "BKSP",  0x2A },
    { "BACKSPACE",0x2A },
    { "DEL",   0x4C },
    { "DELETE",0x4C },
    { "SPACE", 0x2C },
    { "F1", 0x3A }, { "F2", 0x3B }, { "F3", 0x3C }, { "F4", 0x3D },
    { "F5", 0x3E }, { "F6", 0x3F }, { "F7", 0x40 }, { "F8", 0x41 },
    { "F9", 0x42 }, { "F10", 0x43 }, { "F11", 0x44 }, { "F12", 0x45 }
    // Aggiungi altri se necessario
};

QVector<KeyAction> PlaceholderParser::parse(const QString &input, QString *errorMsg)
{
    if (errorMsg) *errorMsg = {};

    // Step 1: gestisci doppie { }
    QString src = input;
    src.replace("{{", QString(QChar(0x01)));
    src.replace("}}", QString(QChar(0x02)));

    QVector<KeyAction> actions;
    QString buffer;
    bool inPlaceholder = false;

    for (int i = 0; i < src.size(); ++i) {
        QChar ch = src.at(i);
        if (ch == '{') {
            if (inPlaceholder) {
                if (errorMsg) *errorMsg = "Nested '{' non supportata";
                return {};
            }
            // flush buffer plain text
            if (!buffer.isEmpty()) {
                appendText(buffer, actions, errorMsg);
                if (errorMsg && !errorMsg->isEmpty())
                    return {};
                buffer.clear();
            }
            inPlaceholder = true;
        } else if (ch == '}') {
            if (!inPlaceholder) {
                if (errorMsg) *errorMsg = "'}' senza '{'";
                return {};
            }
            // process token
            const QString token = buffer.trimmed();
            buffer.clear();
            inPlaceholder = false;

            KeyAction ka;
            if (!parsePlaceholder(token, ka, errorMsg)) {
                return {};
            }
            actions.append(ka);
        } else {
            buffer.append(ch);
        }
    }

    if (inPlaceholder) {
        if (errorMsg) *errorMsg = "Manca '}' di chiusura";
        return {};
    }
    if (!buffer.isEmpty()) {
        appendText(buffer, actions, errorMsg);
        if (errorMsg && !errorMsg->isEmpty())
            return {};
    }

    // Ripristino parentesi letterali nei segmenti text (già tradotte in azioni)
    // (Le parentesi nei placeholder sono già state processate quindi qui nulla da fare)

    return actions;
}

bool PlaceholderParser::parsePlaceholder(const QString &token, KeyAction &out, QString *errorMsg)
{
    if (token.isEmpty()) {
        if (errorMsg) *errorMsg = "Placeholder vuoto {}";
        return false;
    }

    // DELAY
    if (token.startsWith("DELAY:", Qt::CaseInsensitive)) {
        bool ok = false;
        int ms = token.mid(6).trimmed().toInt(&ok);
        if (!ok || ms <= 0) {
            if (errorMsg) *errorMsg = "DELAY non valido: " + token;
            return false;
        }
        out.delayMs = ms;
        return true;
    }

    // RAW (byte custom)
    if (token.startsWith("RAW:", Qt::CaseInsensitive)) {
        QString val = token.mid(4).trimmed();
        bool ok = false;
        int v = val.startsWith("0x") ? val.toInt(&ok,16) : val.toInt(&ok,10);
        if (!ok || v < 0 || v > 255) {
            if (errorMsg) *errorMsg = "RAW non valido: " + token;
            return false;
        }
        out.keycode = static_cast<quint8>(v);
        out.isRaw = true;
        return true;
    }

    // Combinazioni / tasto singolo
    return parseCombo(token, out, errorMsg);
}

bool PlaceholderParser::parseCombo(const QString &token, KeyAction &out, QString *errorMsg)
{
    const QStringList parts = token.split('+', Qt::SkipEmptyParts);
    if (parts.isEmpty()) {
        if (errorMsg) *errorMsg = "Placeholder vuoto";
        return false;
    }
    quint8 modifiers = 0;
    for (int i = 0; i < parts.size(); ++i) {
        const QString up = parts.at(i).trimmed().toUpper();
        quint8 kc = 0;
        bool isMod = false;
        if (!mapSingleKey(up, kc, isMod)) {
            if (errorMsg) *errorMsg = "Token non riconosciuto: " + up;
            return false;
        }
        if (isMod) {
            modifiers |= kc;
        } else {
            if (i != parts.size() - 1) {
                if (errorMsg) *errorMsg = "Il tasto principale deve essere l'ultimo: " + token;
                return false;
            }
            out.keycode = kc;
        }
    }
    out.modifiers = modifiers;
    return true;
}

bool PlaceholderParser::mapSingleKey(const QString &nameUpper, quint8 &keycode, bool &isModifier)
{
    if (s_modMap.contains(nameUpper)) {
        keycode = s_modMap.value(nameUpper);
        isModifier = true;
        return true;
    }
    if (s_keyMap.contains(nameUpper)) {
        keycode = s_keyMap.value(nameUpper);
        isModifier = false;
        return true;
    }

    // Lettera singola A-Z
    if (nameUpper.size() == 1 && nameUpper[0].isLetter()) {
        // HID: 'a' = 0x04
        keycode = 0x04 + (nameUpper[0].unicode() - 'A');
        isModifier = false;
        return true;
    }

    return false;
}

void PlaceholderParser::appendText(const QString &text, QVector<KeyAction>& actions, QString *errorMsg)
{
    for (QChar c : text) {
        if (c.unicode() == 0x01) { c = '{'; }
        else if (c.unicode() == 0x02) { c = '}'; }

        quint8 modifiers = 0;
        bool ok = false;
        quint8 kc = charToKeycode(c, modifiers, ok);
        if (!ok) {
            if (errorMsg) *errorMsg = QStringLiteral("Carattere non mappabile: '%1' (U+%2)")
                                .arg(c).arg(QString::number(c.unicode(),16).toUpper());
            return;
        }
        KeyAction ka;
        ka.modifiers = modifiers;
        ka.keycode = kc;
        actions.append(ka);
    }
}

quint8 PlaceholderParser::charToKeycode(QChar c, quint8 &modifiers, bool &ok)
{
    modifiers = 0;
    ok = true;
    if (c.isLetter()) {
        bool upper = c.isUpper();
        QChar base = c.toUpper();
        quint8 kc = 0x04 + (base.unicode() - 'A');
        if (upper)
            modifiers |= 0x02; // SHIFT
        return kc;
    }
    if (c.isDigit()) {
        // '1' => HID 0x1E
        return 0x1E + (c.unicode() - '1'); // NB: '0' è speciale
    }
    switch (c.unicode()) {
    case '0': return 0x27;
    case ' ': return 0x2C;
    case '-': return 0x2D;
    case '=': return 0x2E;
    case '[': return 0x2F;
    case ']': return 0x30;
    case '\\': return 0x31;
    case ';': return 0x33;
    case '\'': return 0x34;
    case '`': return 0x35;
    case ',': return 0x36;
    case '.': return 0x37;
    case '/': return 0x38;
    default:
        ok = false;
        return 0;
    }
}

QByteArray PlaceholderParser::buildBlePayload(const QVector<KeyAction>& actions)
{
    QByteArray out;
    out.reserve(actions.size() * 4);

    for (const KeyAction &a : actions) {
        if (a.delayMs > 0) {
            // Esempio: 0x02 = tipo delay + ms LE (16 bit) (adatta alla tua convenzione)
            out.append(char(0x02));
            quint16 ms = static_cast<quint16>(qMin(a.delayMs, 65535));
            out.append(char(ms & 0xFF));
            out.append(char((ms >> 8) & 0xFF));
            continue;
        }
        // Esempio: 0x01 = evento key (press+release implicito)
        // Formato ipotetico: [0x01][modifiers][keycode]
        out.append(char(0x01));
        out.append(char(a.modifiers));
        out.append(char(a.keycode));
    }

    return out;
}
