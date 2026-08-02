#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>
#include <QCloseEvent>
#include <QList>
#include <QFutureWatcher>
#include "AppItem.hpp"

class QAction;
class QTableView;
class QStandardItemModel;
class QLabel;
class QProgressBar;
class QComboBox;
class QLineEdit;
class QPushButton;
class QCheckBox;
class QTabWidget;
class QFileIconProvider;
class LangueManager;
class AppDetector;
class VersionChecker;
class Downloader;
class Installer;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void analyze_all();
    void refresh_online();
    void open_homepage(const QModelIndex& index);
    void selection_changed();
    void select_all_outdated();
    void apply_action_phase2();
    void download_selected(const QList<AppItem*>& selected);
    void create_command_file(const QList<AppItem*>& selected);
    void install_selected(const QList<AppItem*>& selected);
    void repair_selected(const QList<AppItem*>& selected);
    void repair_incorrect_install();
    void filter_changed();
    void on_app_checked(const QString& appId);
    void on_all_checked();
    void on_detection_done();
    void on_check_error(const QString& appId, const QString& error);
    void on_download_progress(const QString& appId, qint64 received, qint64 total);
    void on_download_finished(const QString& appId, bool success, const QString& filePath);
    void on_install_finished(const QString& appId, bool success, const QString& output);
    void changer_langue(const QString& langCode);
    void retranslateUi();
    void show_about();

private:
    void create_menus();
    void create_toolbar();
    void create_central();
    void load_settings();
    void save_settings();
    void download_language(const QString& code);
    void populate_table();
    void update_row(int row);
    void update_table_all();
    void apply_filter();
    void rebuild_tabs();
    void set_status_message(const QString& msg);

    // Composants
    LangueManager* langue_ = nullptr;
    AppDetector* detector_ = nullptr;
    VersionChecker* checker_ = nullptr;
    Downloader* downloader_ = nullptr;
    Installer* installer_ = nullptr;

    // Données
    QList<AppItem> apps_;
    QString apps_path_;
    QFutureWatcher<void>* detect_watcher_ = nullptr;

    // UI
    QTableView* table_ = nullptr;
    QStandardItemModel* model_ = nullptr;
    QFileIconProvider* icon_provider_ = nullptr;
    QLabel* status_label_ = nullptr;
    QProgressBar* progress_bar_ = nullptr;
    QComboBox* filter_status_ = nullptr;
    QTabWidget* tabs_ = nullptr;
    QLineEdit* search_box_ = nullptr;
    QCheckBox* cb_download_ = nullptr;
    QCheckBox* cb_command_ = nullptr;
    QCheckBox* cb_install_ = nullptr;
    QCheckBox* cb_repair_ = nullptr;
    QPushButton* btn_actions_ = nullptr;
    QPushButton* btn_repair_ = nullptr;

    // Menus
    QMenu* menu_fichier_ = nullptr;
    QMenu* menu_outils_ = nullptr;
    QMenu* menu_langue_ = nullptr;
    QMenu* menu_aide_ = nullptr;
    QAction* a_analyze_ = nullptr;
    QAction* a_refresh_ = nullptr;
    QAction* a_quitter_ = nullptr;
    QAction* a_about_ = nullptr;
};

#endif
