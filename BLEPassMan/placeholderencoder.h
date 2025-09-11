#ifndef PLACEHOLDERENCODER_H
#define PLACEHOLDERENCODER_H

#include <QString>
#include <QByteArray>
#include <QHash>

class PlaceholderEncoder {
public:
    // Converte stringa con {ENTER}{DELAY:1000}... in raw bytes (ASCII + 0x80–0x87)
    // Ritorna QByteArray vuota se errore (errorMsg valorizzato).
    static QByteArray encode(const QString &input, QString *errorMsg = nullptr);

    // Converte raw bytes (inclusi 0x80–0x87) in stringa con placeholder testuali.
    static QString decode(const QByteArray &raw);

private:
    static bool parsePlaceholderToken(const QString &token, char &outByte, QString *errorMsg);
    static void initMaps();
    static QHash<QString, char> s_tokenToByte;
    static QHash<char, QString> s_byteToToken;
};

#endif // PLACEHOLDERENCODER_H
