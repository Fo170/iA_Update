#ifndef APPITEM_HPP
#define APPITEM_HPP

#include <QString>
#include <QList>
#include <QJsonObject>

// Statut global d'une application après analyse
enum class AppStatus {
    NotInstalled,   // 🔴 non installé
    UpToDate,       // 🟢 à jour
    UpdateAvailable,// 🟠 mise à jour disponible
    UnknownLocal,   // ⚪ installé mais version locale inconnue
    UnknownOnline,  // 🔵 installé, version en ligne inconnue
    Error           // ❌ erreur de détection / de lecture
};

// Portée de l'installation
enum class AppScope {
    Global,   // installé pour tous les utilisateurs
    User,     // installé pour l'utilisateur courant
    Unknown
};

class AppItem {
public:
    AppItem();

    // Remplit l'item depuis un objet JSON du manifeste (apps.json)
    static AppItem fromJson(const QJsonObject& obj);
    // Charge le manifeste complet depuis un fichier JSON
    static QList<AppItem> loadManifest(const QString& path, QString* error = nullptr);
    // Applique les surcharges de commandes depuis un fichier INI éditable
    // (commandes.ini) : clés [appid]/windows_update, linux_update, windows_repair, linux_repair
    static void applyIniOverrides(QList<AppItem>& items, const QString& iniPath);
    // Génère le fichier INI de commandes à partir des valeurs actuelles
    static void writeIniFile(const QList<AppItem>& items, const QString& iniPath);
    QJsonObject toJson() const;

    // ── Champs statiques (manifeste) ─────────────────────────────
    QString id;
    QString name;
    QString category;
    QString homepage;
    QString scope_default;
    QString downloadUrl;
    QJsonObject detect;
    QJsonObject online;
    QJsonObject updateCommand;
    QJsonObject repairCommand;
    QString versionLocalRegex;      // regex appliquée à la sortie de la commande
    QString versionLocalRegistry;   // sous-chaîne du DisplayName (registre Windows)
    QString versionLocalFromPath;   // regex appliquée au chemin d'installation

    // ── Champs résultats (analyse) ────────────────────────────────
    bool installed = false;
    bool referenced_in_path = false;  // binaire trouvé dans le PATH
    QString install_path;             // chemin d'installation détecté
    QString local_version;            // version locale lue
    QString online_version;           // version en ligne lue
    bool online_checked = false;      // la vérif en ligne a été tentée
    bool online_error = false;        // échec de la vérif en ligne
    QString error_message;
    AppStatus status = AppStatus::NotInstalled;
    AppScope scope = AppScope::Unknown;

    // Helpers
    bool is_selected = false;         // case à cocher phase 2
    void computeStatus();
    QString statusKey() const;        // clé i18n pour le statut
    QString scopeKey() const;         // clé i18n pour la portée

    // Retourne la liste des commandes de détection pour l'OS courant
    QStringList detectCommands() const;
    // Retourne la commande de localisation PATH (detect.locate) si définie
    QStringList detectLocateCommand() const;
    // Retourne la liste des chemins de détection pour l'OS courant
    QStringList detectPaths() const;
    // Retourne la commande de mise à jour pour l'OS courant
    QString updateCommandForOS() const;
    // Retourne la commande de réparation pour l'OS courant
    QString repairCommandForOS() const;
};

#endif
