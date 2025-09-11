#include "placeholderencoder.h"

// Placeholder definiti nel firmware (password_placeholders.h)
static constexpr char PW_PH_ENTER        = char(0x80);
static constexpr char PW_PH_TAB          = char(0x81);
static constexpr char PW_PH_ESC          = char(0x82);
static constexpr char PW_PH_BACKSPACE    = char(0x83);
static constexpr char PW_PH_DELAY_500MS  = char(0x84);
static constexpr char PW_PH_DELAY_1000MS = char(0x85);
static constexpr char PW_PH_CTRL_ALT_DEL = char(0x86);
static constexpr char PW_PH_SHIFT_TAB    = char(0x87);

QHash<QString, char> PlaceholderEncoder::s_tokenToByte;
QHash<char, QString> PlaceholderEncoder::s_byteToToken;

void PlaceholderEncoder::initMaps() {
    if (!s_tokenToByte.isEmpty()) return;
    struct Entry { const char* name; char b; };
    const Entry entries[] = {
        {"ENTER",        PW_PH_ENTER},
        {"TAB",          PW_PH_TAB},
        {"ESC",          PW_PH_ESC},
        {"ESCAPE",       PW_PH_ESC},
        {"BACKSPACE",    PW_PH_BACKSPACE},
        {"BKSP",         PW_PH_BACKSPACE},
        {"DELAY:500",    PW_PH_DELAY_500MS},
        {"DELAY:1000",   PW_PH_DELAY_1000MS},
        {"CTRL+ALT+DEL", PW_PH_CTRL_ALT_DEL},
        {"SHIFT+TAB",    PW_PH_SHIFT_TAB}
    };
    for (auto &e : entries) {
        QString key = QString::fromLatin1(e.name).toUpper();
        s_tokenToByte.insert(key, e.b);
        if (!s_byteToToken.contains(e.b))
            s_byteToToken.insert(e.b, key);
    }
}

QByteArray PlaceholderEncoder::encode(const QString &input, QString *errorMsg)
{
    if (errorMsg) *errorMsg = {};
    initMaps();

    QByteArray out;
    out.reserve(input.size());

    QString src = input;
    // Escaping graffe
    src.replace("{{", QString(QChar(0x01)));
    src.replace("}}", QString(QChar(0x02)));

    QString tokenBuffer;
    bool inPh = false;

    for (int i=0; i<src.size(); ++i) {
        QChar ch = src.at(i);
        if (ch == '{') {
            if (inPh) { if (errorMsg) *errorMsg = "Placeholder annidato non supportato"; return {}; }
            inPh = true;
            tokenBuffer.clear();
            continue;
        }
        if (ch == '}') {
            if (!inPh) { if (errorMsg) *errorMsg = "Trovato '}' senza '{'"; return {}; }
            inPh = false;
            QString token = tokenBuffer.trimmed().toUpper();
            char phByte=0;
            if (!parsePlaceholderToken(token, phByte, errorMsg)) return {};
            out.append(phByte);
            tokenBuffer.clear();
            continue;
        }
        if (inPh) {
            tokenBuffer.append(ch);
        } else {
            if (ch.unicode()==0x01) { out.append('{'); }
            else if (ch.unicode()==0x02) { out.append('}'); }
            else {
                if (ch.unicode() <= 0x7F) {
                    out.append(char(ch.unicode() & 0x7F));
                } else {
                    if (errorMsg) *errorMsg = QString("Carattere non ASCII non supportato: U+%1")
                                        .arg(QString::number(ch.unicode(),16).toUpper());
                    return {};
                }
            }
        }
    }
    if (inPh) { if (errorMsg) *errorMsg = "Manca '}' di chiusura"; return {}; }
    return out;
}

bool PlaceholderEncoder::parsePlaceholderToken(const QString &token, char &outByte, QString *errorMsg)
{
    if (token.isEmpty()) { if (errorMsg) *errorMsg = "Placeholder vuoto {}"; return false; }

    if (token.startsWith("DELAY:", Qt::CaseInsensitive)) {
        bool ok=false; int ms = token.mid(6).trimmed().toInt(&ok);
        if (!ok) { if (errorMsg) *errorMsg = "Valore DELAY non numerico"; return false; }
        if (ms==500)  { outByte = PW_PH_DELAY_500MS; return true; }
        if (ms==1000) { outByte = PW_PH_DELAY_1000MS; return true; }
        if (errorMsg) *errorMsg = QString("DELAY %1 ms non supportato (solo 500 o 1000)").arg(ms);
        return false;
    }

    QString norm = token;
    norm.replace(' ', "");
    norm = norm.toUpper();

    if (s_tokenToByte.contains(norm)) {
        outByte = s_tokenToByte.value(norm);
        return true;
    }
    if (errorMsg) *errorMsg = "Placeholder non riconosciuto: {" + token + "}";
    return false;
}

QString PlaceholderEncoder::decode(const QByteArray &raw)
{
    initMaps();
    QString out;
    out.reserve(raw.size()*6);
    for (unsigned char b : raw) {
        if (s_byteToToken.contains(char(b))) {
            out += '{' + s_byteToToken.value(char(b)) + '}';
        } else if (b=='{') {
            out += "{{";
        } else if (b=='}') {
            out += "}}";
        } else if (b >= 0x20 && b < 0x7F) {
            out += QChar(b);
        } else if (b == 0x00) {
            // padding: ignorato in decode (non aggiungo nulla)
        } else {
            // byte non stampabile / futuro placeholder
            out += QString("\\x%1").arg(QString::number(b,16).toUpper().rightJustified(2,'0'));
        }
    }
    return out;
}
