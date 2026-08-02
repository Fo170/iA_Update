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
    QString versionLocalRegex;

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
    // Retourne la liste des chemins de détection pour l'OS courant
    QStringList detectPaths() const;
    // Retourne la commande de mise à jour pour l'OS courant
    QString updateCommandForOS() const;
};

#endif
