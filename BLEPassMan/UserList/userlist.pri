# Aggiunge Qt Quick come dipendenza
QT += qml

# Aggiunge la cartella corrente al percorso di importazione di QML
# così da poter fare "import UserList"
QML_IMPORT_PATH += $$PWD

# Specifica il file delle risorse QML da includere
RESOURCES += \
    $$PWD/userlist.qrc