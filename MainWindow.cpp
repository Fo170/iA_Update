#include "MainWindow.hpp"
#include "AppConfig.hpp"
#include "LangueManager.hpp"
#include "AppDetector.hpp"
#include "VersionChecker.hpp"
#include "Downloader.hpp"
#include "CommandBuilder.hpp"
#include "Installer.hpp"

#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QToolBar>
#include <QStatusBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QTableView>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QHeaderView>
#include <QProgressBar>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QDesktopServices>
#include <QUrl>
#include <QSettings>
#include <QDir>
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QFileIconProvider>
#include <QStyle>

// Colonnes du tableau
enum {
    COL_ICON = 0,
    COL_SELECT,
    COL_NAME,
    COL_LOCAL,
    COL_ONLINE,
    COL_STATUS,
    COL_SCOPE,
    COL_PATH,
    COL_PATH_REF,
    COL_COUNT
};

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    resize(1100, 700);

    langue_ = new LangueManager(
        QCoreApplication::applicationDirPath() + "/lang", this);
    detector_ = new AppDetector();
    checker_ = new VersionChecker(this);
    downloader_ = new Downloader(this);
    installer_ = new Installer(this);
    icon_provider_ = new QFileIconProvider;

    // Chemin du manifeste apps.json (à côté de l'exécutable)
    apps_path_ = QCoreApplication::applicationDirPath() + "/apps.json";
    if (!QFile::exists(apps_path_))
        apps_path_ = QCoreApplication::applicationDirPath() + "/../apps.json";

    load_settings();

    connect(checker_, &VersionChecker::appChecked, this, &MainWindow::on_app_checked);
    connect(checker_, &VersionChecker::allChecked, this, &MainWindow::on_all_checked);
    connect(checker_, &VersionChecker::checkError, this, &MainWindow::on_check_error);
    connect(downloader_, &Downloader::downloadProgress, this, &MainWindow::on_download_progress);
    connect(downloader_, &Downloader::downloadFinished, this, &MainWindow::on_download_finished);
    connect(installer_, &Installer::installFinished, this, &MainWindow::on_install_finished);

    create_menus();
    create_toolbar();
    create_central();

    retranslateUi();
    statusBar()->showMessage(langue_->get("status.ready"));

    analyze_all();
}

void MainWindow::create_menus() {
    menu_fichier_ = menuBar()->addMenu(QString());
    menu_outils_ = menuBar()->addMenu(QString());
    menu_langue_ = menuBar()->addMenu(QString());
    menu_aide_ = menuBar()->addMenu(QString());

    a_analyze_ = menu_fichier_->addAction(QString());
    a_analyze_->setShortcut(QKeySequence::Refresh);
    connect(a_analyze_, &QAction::triggered, this, &MainWindow::analyze_all);

    a_refresh_ = menu_fichier_->addAction(QString());
    a_refresh_->setShortcut(QKeySequence(QStringLiteral("Ctrl+R")));
    connect(a_refresh_, &QAction::triggered, this, &MainWindow::refresh_online);

    menu_fichier_->addSeparator();
    a_quitter_ = menu_fichier_->addAction(QString());
    a_quitter_->setShortcut(QKeySequence::Quit);
    connect(a_quitter_, &QAction::triggered, this, &QWidget::close);

    connect(langue_, &LangueManager::languageChanged, this, &MainWindow::retranslateUi);
}

void MainWindow::create_toolbar() {
    auto* tb = addToolBar(QStringLiteral("main"));
    tb->setIconSize(QSize(16, 16));
    tb->addAction(a_analyze_);
    tb->addAction(a_refresh_);
}

void MainWindow::create_central() {
    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);

    // Onglets par catégorie
    tabs_ = new QTabWidget;
    tabs_->setDocumentMode(true);
    layout->addWidget(tabs_);

    // Barre de filtres
    auto* filter_bar = new QHBoxLayout;

    filter_status_ = new QComboBox;
    filter_status_->addItem(QString(), QString());
    search_box_ = new QLineEdit;
    search_box_->setClearButtonEnabled(true);
    search_box_->setPlaceholderText(QString());

    filter_bar->addWidget(filter_status_);
    filter_bar->addWidget(search_box_, 1);

    // Bouton de sélection
    auto* btn_select = new QPushButton;
    btn_select->setText(QStringLiteral("✓"));
    btn_select->setToolTip(QString());
    filter_bar->addWidget(btn_select);

    layout->addLayout(filter_bar);

    connect(filter_status_, &QComboBox::currentIndexChanged,
            this, &MainWindow::filter_changed);
    connect(search_box_, &QLineEdit::textChanged,
            this, &MainWindow::filter_changed);
    connect(btn_select, &QPushButton::clicked,
            this, &MainWindow::select_all_outdated);

    // Tableau
    table_ = new QTableView;
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setAlternatingRowColors(true);
    table_->verticalHeader()->setVisible(false);

    model_ = new QStandardItemModel(this);
    model_->setColumnCount(COL_COUNT);
    table_->setModel(model_);
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    connect(table_, &QTableView::clicked, this, &MainWindow::open_homepage);

    layout->addWidget(table_, 1);

    // Barre de progression
    progress_bar_ = new QProgressBar;
    progress_bar_->setRange(0, 100);
    progress_bar_->setValue(0);
    progress_bar_->setVisible(false);
    layout->addWidget(progress_bar_);

    // Phase 2 : groupe d'actions
    auto* phase2 = new QGroupBox;
    auto* p2 = new QHBoxLayout(phase2);

    cb_download_ = new QCheckBox;
    cb_command_ = new QCheckBox;
    cb_install_ = new QCheckBox;
    cb_repair_ = new QCheckBox;
    btn_actions_ = new QPushButton;
    btn_repair_ = new QPushButton;

    p2->addWidget(cb_download_);
    p2->addWidget(cb_command_);
    p2->addWidget(cb_install_);
    p2->addWidget(cb_repair_);
    p2->addStretch(1);
    p2->addWidget(btn_repair_);
    p2->addWidget(btn_actions_);

    layout->addWidget(phase2);

    connect(cb_download_, &QCheckBox::checkStateChanged, this, &MainWindow::selection_changed);
    connect(cb_command_, &QCheckBox::checkStateChanged, this, &MainWindow::selection_changed);
    connect(cb_install_, &QCheckBox::checkStateChanged, this, &MainWindow::selection_changed);
    connect(cb_repair_, &QCheckBox::checkStateChanged, this, &MainWindow::selection_changed);
    connect(btn_actions_, &QPushButton::clicked, this, &MainWindow::apply_action_phase2);
    connect(btn_repair_, &QPushButton::clicked, this, &MainWindow::repair_incorrect_install);

    status_label_ = new QLabel;
    statusBar()->addWidget(status_label_, 1);

    setCentralWidget(central);
}

void MainWindow::populate_table() {
    model_->clear();
    model_->setRowCount(apps_.size());

    // Remplissage
    for (int i = 0; i < apps_.size(); ++i) {
        AppItem& app = apps_[i];
        auto* icon = new QStandardItem;
        icon->setEditable(false);
        icon->setTextAlignment(Qt::AlignCenter);
        model_->setItem(i, COL_ICON, icon);
        auto* ck = new QStandardItem;
        ck->setCheckable(true);
        ck->setEditable(false);
        model_->setItem(i, COL_SELECT, ck);
        model_->setItem(i, COL_NAME, new QStandardItem(app.name));
        model_->setItem(i, COL_LOCAL, new QStandardItem(app.local_version));
        model_->setItem(i, COL_ONLINE, new QStandardItem(app.online_version));
        model_->setItem(i, COL_STATUS, new QStandardItem);
        model_->setItem(i, COL_SCOPE, new QStandardItem);
        model_->setItem(i, COL_PATH, new QStandardItem(app.install_path));
        model_->setItem(i, COL_PATH_REF, new QStandardItem);
        update_row(i);
    }

    // En-têtes
    QStringList headers;
    headers << QString() << QString() << langue_->get("col.name") << langue_->get("col.local")
            << langue_->get("col.online") << langue_->get("col.status")
            << langue_->get("col.scope") << langue_->get("col.path")
            << langue_->get("col.path_ref");
    model_->setHorizontalHeaderLabels(headers);

    apply_filter();
}

void MainWindow::update_row(int row) {
    if (row < 0 || row >= apps_.size())
        return;
    const AppItem& app = apps_[row];

    QStandardItem* ck = model_->item(row, COL_SELECT);
    if (ck) ck->setCheckState(app.is_selected ? Qt::Checked : Qt::Unchecked);

    // Icône de l'application (exe installé si possible)
    if (auto* icon = model_->item(row, COL_ICON)) {
        QIcon ic;
        if (app.installed && !app.install_path.isEmpty()) {
            QString exePath = app.install_path;
            // Si le chemin est un dossier (cas PATH), on cherche un .exe dedans
            if (QFileInfo(exePath).isDir()) {
                QDir d(exePath);
                QStringList exes = d.entryList({"*.exe"}, QDir::Files);
                if (!exes.isEmpty())
                    exePath = d.absoluteFilePath(exes.first());
            }
            ic = icon_provider_->icon(QFileInfo(exePath));
            if (ic.isNull())
                ic = style()->standardIcon(QStyle::SP_ComputerIcon);
        } else {
            ic = style()->standardIcon(QStyle::SP_FileDialogNewFolder);
        }
        icon->setIcon(ic);
        icon->setSizeHint(QSize(22, 22));
    }

    if (auto* name = model_->item(row, COL_NAME)) {
        name->setText(app.name);
        name->setData(app.homepage, Qt::UserRole);
    }
    if (auto* local = model_->item(row, COL_LOCAL))
        local->setText(app.local_version);
    if (auto* online = model_->item(row, COL_ONLINE))
        online->setText(app.online_version);

    // Statut avec couleur
    QColor color;
    switch (app.status) {
        case AppStatus::UpToDate:         color = QColor(0, 150, 0);  break;
        case AppStatus::UpdateAvailable:  color = QColor(220, 130, 0); break;
        case AppStatus::NotInstalled:     color = QColor(200, 40, 40); break;
        case AppStatus::UnknownLocal:     color = QColor(130, 130, 130); break;
        case AppStatus::UnknownOnline:    color = QColor(0, 120, 200); break;
        case AppStatus::Error:            color = QColor(200, 40, 40); break;
    }
    QString status_text = langue_->get(app.statusKey());

    if (auto* st = model_->item(row, COL_STATUS)) {
        st->setText(status_text);
        st->setForeground(QBrush(color));
    }
    if (auto* scope = model_->item(row, COL_SCOPE))
        scope->setText(langue_->get(app.scopeKey()));
    if (auto* path = model_->item(row, COL_PATH))
        path->setText(app.install_path);

    QString ref_text;
    QColor ref_color = QColor(200, 40, 40);
    if (app.installed && app.referenced_in_path) {
        ref_text = langue_->get("path.yes");
        ref_color = QColor(0, 150, 0);
    } else if (app.installed) {
        ref_text = langue_->get("path.no");
    } else {
        ref_text = QStringLiteral("—");
    }
    if (auto* ref = model_->item(row, COL_PATH_REF)) {
        ref->setText(ref_text);
        ref->setForeground(QBrush(ref_color));
    }
}

void MainWindow::apply_filter() {
    if (!model_) return;

    int stIdx = filter_status_->currentIndex();
    QString stData = filter_status_->itemData(stIdx).toString();
    QString catData = tabs_ ? tabs_->currentIndex() > 0
                                ? tabs_->widget(tabs_->currentIndex())->objectName()
                                : QString()
                            : QString();
    QString search = search_box_->text().toLower();

    for (int row = 0; row < apps_.size(); ++row) {
        const AppItem& app = apps_[row];
        bool show = true;

        if (!stData.isEmpty() && app.statusKey() != stData)
            show = false;
        if (show && !catData.isEmpty() && app.category != catData)
            show = false;
        if (show && !search.isEmpty() &&
            !app.name.toLower().contains(search) &&
            !app.id.toLower().contains(search))
            show = false;

        table_->setRowHidden(row, !show);
    }
}

void MainWindow::rebuild_tabs() {
    if (!tabs_) return;

    // Sauvegarde de la catégorie courante
    QString current = tabs_->currentIndex() > 0
        ? tabs_->widget(tabs_->currentIndex())->objectName() : QString();

    while (tabs_->count() > 0)
        tabs_->removeTab(0);

    auto* allTab = new QWidget;
    tabs_->addTab(allTab, langue_->get("tab.all"));
    QStringList cats = {"build", "outils", "local", "assistants", "code"};
    int restore = 0;
    for (const QString& c : cats) {
        auto* page = new QWidget;
        page->setObjectName(c);
        tabs_->addTab(page, langue_->get("tab." + c));
        int idx = tabs_->count() - 1;
        if (c == current)
            restore = idx;
    }
    tabs_->setCurrentIndex(restore);
    connect(tabs_, &QTabWidget::currentChanged, this, &MainWindow::filter_changed,
            Qt::UniqueConnection);
}

void MainWindow::analyze_all() {
    if (checker_->isChecking()) {
        set_status_message(langue_->get("status.checking"));
        return;
    }

    // Chargement du manifeste
    apps_ = AppItem::loadManifest(apps_path_);
    if (apps_.isEmpty()) {
        QMessageBox::warning(this, langue_->get("app.name"),
            langue_->get("status.manifest_missing") + "\n" + apps_path_);
        return;
    }

    // Détection locale
    progress_bar_->setVisible(true);
    progress_bar_->setRange(0, apps_.size());
    progress_bar_->setValue(0);
    set_status_message(langue_->get("status.detecting"));

    for (int i = 0; i < apps_.size(); ++i) {
        detector_->detect(apps_[i]);
        progress_bar_->setValue(i + 1);
    }

    // Remplissage des onglets par catégorie
    rebuild_tabs();

    filter_status_->blockSignals(true);
    filter_status_->clear();
    filter_status_->addItem(langue_->get("filter.all"), QString());
    QStringList statusKeys = {
        QStringLiteral("status.not_installed"),
        QStringLiteral("status.update_available"),
        QStringLiteral("status.up_to_date"),
        QStringLiteral("status.unknown_local"),
        QStringLiteral("status.unknown_online")
    };
    for (const QString& k : statusKeys) {
        filter_status_->addItem(langue_->get(k), k);
    }
    filter_status_->blockSignals(false);

    populate_table();

    // Lancement de la vérification en ligne
    refresh_online();
}

void MainWindow::refresh_online() {
    if (apps_.isEmpty())
        return;
    if (checker_->isChecking())
        return;

    progress_bar_->setRange(0, 100);
    progress_bar_->setValue(0);
    set_status_message(langue_->get("status.checking"));
    checker_->checkAll(apps_);
}

void MainWindow::on_app_checked(const QString& appId) {
    Q_UNUSED(appId);
    // Mise à jour visuelle de la ligne concernée
    for (int row = 0; row < apps_.size(); ++row) {
        if (apps_[row].id == appId) {
            update_row(row);
            break;
        }
    }
}

void MainWindow::on_all_checked() {
    progress_bar_->setVisible(false);
    set_status_message(langue_->get("status.ready"));
    update_table_all();
}

void MainWindow::update_table_all() {
    for (int row = 0; row < apps_.size(); ++row)
        update_row(row);
    apply_filter();
}

void MainWindow::on_check_error(const QString& appId, const QString& error) {
    Q_UNUSED(appId);
    set_status_message(langue_->get("status.check_error") + " : " + error);
}

void MainWindow::open_homepage(const QModelIndex& index) {
    if (index.column() != COL_NAME)
        return;
    const AppItem& app = apps_[index.row()];
    if (!app.homepage.isEmpty())
        QDesktopServices::openUrl(QUrl(app.homepage));
}

void MainWindow::selection_changed() {
    // On recueille les sélections du tableau
    for (int row = 0; row < apps_.size(); ++row) {
        auto* ck = model_->item(row, COL_SELECT);
        if (ck)
            apps_[row].is_selected = (ck->checkState() == Qt::Checked);
    }
    btn_actions_->setEnabled(cb_download_->isChecked() || cb_command_->isChecked() ||
                             cb_install_->isChecked() || cb_repair_->isChecked());
}

void MainWindow::select_all_outdated() {
    for (int row = 0; row < apps_.size(); ++row) {
        if (apps_[row].status == AppStatus::UpdateAvailable) {
            apps_[row].is_selected = true;
            if (auto* ck = model_->item(row, COL_SELECT))
                ck->setCheckState(Qt::Checked);
        }
    }
}

void MainWindow::apply_action_phase2() {
    // Collecte des apps sélectionnées
    QList<AppItem*> selected;
    for (int row = 0; row < apps_.size(); ++row) {
        if (apps_[row].is_selected && apps_[row].installed)
            selected.append(&apps_[row]);
    }

    if (selected.isEmpty()) {
        set_status_message(langue_->get("status.none_selected"));
        return;
    }

    if (cb_download_->isChecked())
        download_selected(selected);
    if (cb_command_->isChecked())
        create_command_file(selected);
    if (cb_install_->isChecked())
        install_selected(selected);
    if (cb_repair_->isChecked())
        repair_selected(selected);
}

void MainWindow::repair_selected(const QList<AppItem*>& selected) {
    // Confirmation
    QMessageBox mb(this);
    mb.setWindowTitle(langue_->get("dialog.repair.title"));
    mb.setText(langue_->get("dialog.repair.confirm"));
    mb.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    mb.button(QMessageBox::Yes)->setText(langue_->get("dialog.install.yes"));
    mb.button(QMessageBox::No)->setText(langue_->get("dialog.install.no"));
    mb.setDefaultButton(QMessageBox::No);

    if (mb.exec() != QMessageBox::Yes)
        return;
    set_status_message(langue_->get("status.repairing"));
    installer_->repair(selected);
}

void MainWindow::repair_incorrect_install() {
    // Réparation des installations suspectes : installé mais PAS référencé
    // dans le PATH (mauvaise installation, vestige, etc.) ou installé avec
    // une version inconnue. L'utilisateur choisit lesquelles réparer.
    QList<AppItem*> suspicious;
    QStringList names;
    for (int row = 0; row < apps_.size(); ++row) {
        AppItem& app = apps_[row];
        bool suspect = app.installed &&
                       ((!app.referenced_in_path && !app.detectCommands().isEmpty()) ||
                        app.local_version.isEmpty() && !app.versionLocalRegex.isEmpty() &&
                            app.versionLocalRegistry.isEmpty());
        if (suspect) {
            suspicious.append(&app);
            names << app.name;
        }
    }

    if (suspicious.isEmpty()) {
        set_status_message(langue_->get("status.no_repair_needed"));
        return;
    }

    QMessageBox mb(this);
    mb.setWindowTitle(langue_->get("dialog.repair.title"));
    mb.setText(langue_->get("dialog.repair.incorrect") + "\n\n" + names.join("\n"));
    mb.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    mb.button(QMessageBox::Yes)->setText(langue_->get("dialog.install.yes"));
    mb.button(QMessageBox::No)->setText(langue_->get("dialog.install.no"));
    mb.setDefaultButton(QMessageBox::No);

    if (mb.exec() != QMessageBox::Yes)
        return;

    for (auto* app : suspicious)
        app->is_selected = true;

    set_status_message(langue_->get("status.repairing"));
    installer_->repair(suspicious);
}

void MainWindow::download_selected(const QList<AppItem*>& selected) {
    QString destDir = QFileDialog::getExistingDirectory(
        this, langue_->get("dialog.download_dir"),
        QCoreApplication::applicationDirPath() + "/download");
    if (destDir.isEmpty())
        return;
    set_status_message(langue_->get("status.downloading"));
    downloader_->download(selected, destDir);
}

void MainWindow::create_command_file(const QList<AppItem*>& selected) {
    QString ext = CommandBuilder::scriptExtension();
    QString defaultName = QDir(QCoreApplication::applicationDirPath())
                              .filePath(QStringLiteral("mise_a_jour") + ext);
    QString path = QFileDialog::getSaveFileName(
        this, langue_->get("dialog.command_file"), defaultName,
        QStringLiteral("Script (*%1)").arg(ext));
    if (path.isEmpty())
        return;

    CommandBuilder builder;
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        set_status_message(langue_->get("status.error_writing"));
        return;
    }
    f.write(builder.buildScript(selected).toUtf8());
    f.close();
    set_status_message(langue_->get("status.command_created") + " : " + path);
}

void MainWindow::install_selected(const QList<AppItem*>& selected) {
    // Confirmation
    QMessageBox mb(this);
    mb.setWindowTitle(langue_->get("dialog.install.title"));
    mb.setText(langue_->get("dialog.install.confirm"));
    mb.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    mb.button(QMessageBox::Yes)->setText(langue_->get("dialog.install.yes"));
    mb.button(QMessageBox::No)->setText(langue_->get("dialog.install.no"));
    mb.setDefaultButton(QMessageBox::No);

    if (mb.exec() != QMessageBox::Yes)
        return;
    set_status_message(langue_->get("status.installing"));
    installer_->install(selected);
}

void MainWindow::on_download_progress(const QString& appId, qint64 received, qint64 total) {
    Q_UNUSED(appId);
    if (total > 0)
        progress_bar_->setVisible(true);
    if (total > 0)
        progress_bar_->setValue((received * 100) / total);
}

void MainWindow::on_download_finished(const QString& appId, bool success, const QString& filePath) {
    Q_UNUSED(appId);
    progress_bar_->setVisible(false);
    if (success)
        set_status_message(langue_->get("status.download_done") + " : " + filePath);
    else
        set_status_message(langue_->get("status.download_failed") + " : " + filePath);
}

void MainWindow::on_install_finished(const QString& appId, bool success, const QString& output) {
    QString name = appId;
    for (const auto& app : apps_) {
        if (app.id == appId) {
            name = app.name;
            break;
        }
    }
    if (success)
        set_status_message(langue_->get("status.install_done") + " : " + name);
    else
        set_status_message(langue_->get("status.install_failed") + " : " + name + " — " + output);
}

void MainWindow::filter_changed() {
    apply_filter();
}

void MainWindow::set_status_message(const QString& msg) {
    statusBar()->showMessage(msg);
    if (status_label_)
        status_label_->setText(msg);
}

void MainWindow::retranslateUi() {
    menu_fichier_->setTitle(langue_->get("menu.file"));
    a_analyze_->setText(langue_->get("menu.file.analyze"));
    a_refresh_->setText(langue_->get("menu.file.refresh"));
    a_quitter_->setText(langue_->get("menu.file.quit"));
    menu_outils_->setTitle(langue_->get("menu.tools"));
    menu_langue_->setTitle(langue_->get("menu.language"));
    menu_aide_->setTitle(langue_->get("menu.help"));

    if (!a_about_) {
        a_about_ = menu_aide_->addAction(QString());
        connect(a_about_, &QAction::triggered, this, &MainWindow::show_about);
    }
    a_about_->setText(langue_->get("menu.help.about"));

    // Rebuild language submenu
    menu_langue_->clear();
    auto* group = new QActionGroup(this);
    QStringList codes = langue_->availableLanguages();
    QStringList names = langue_->languageDisplayNames();
    QString current = langue_->currentLanguage();

    for (int i = 0; i < codes.size(); ++i) {
        auto* a = menu_langue_->addAction(names.at(i));
        a->setCheckable(true);
        a->setChecked(codes.at(i) == current);
        a->setData(codes.at(i));
        group->addAction(a);
        connect(a, &QAction::triggered, this, [this, code = codes.at(i)]() {
            changer_langue(code);
        });
    }

    // Filtres
    if (filter_status_) {
        int idx = filter_status_->currentIndex();
        for (int i = 0; i < filter_status_->count(); ++i) {
            QString k = filter_status_->itemData(i).toString();
            if (k.isEmpty())
                filter_status_->setItemText(i, langue_->get("filter.all"));
            else
                filter_status_->setItemText(i, langue_->get(k));
        }
        filter_status_->setCurrentIndex(idx);
    }
    search_box_->setPlaceholderText(langue_->get("filter.search"));

    // Onglets
    if (tabs_ && tabs_->count() > 0) {
        int currentCat = tabs_->currentIndex();
        for (int i = 0; i < tabs_->count(); ++i) {
            QString cat = tabs_->widget(i)->objectName();
            if (cat.isEmpty())
                tabs_->setTabText(i, langue_->get("tab.all"));
            else
                tabs_->setTabText(i, langue_->get("tab." + cat));
        }
        tabs_->setCurrentIndex(currentCat);
    }

    // Phase 2
    cb_download_->setText(langue_->get("phase2.download"));
    cb_command_->setText(langue_->get("phase2.command"));
    cb_install_->setText(langue_->get("phase2.install"));
    cb_repair_->setText(langue_->get("phase2.repair"));
    btn_actions_->setText(langue_->get("phase2.apply"));
    btn_actions_->setToolTip(langue_->get("phase2.apply.tip"));
    btn_repair_->setText(langue_->get("phase2.repair_incorrect"));
    btn_repair_->setToolTip(langue_->get("phase2.repair_incorrect.tip"));

    // En-têtes si le modèle existe
    if (model_ && model_->columnCount() == COL_COUNT) {
        model_->setHeaderData(COL_NAME, Qt::Horizontal, langue_->get("col.name"));
        model_->setHeaderData(COL_LOCAL, Qt::Horizontal, langue_->get("col.local"));
        model_->setHeaderData(COL_ONLINE, Qt::Horizontal, langue_->get("col.online"));
        model_->setHeaderData(COL_STATUS, Qt::Horizontal, langue_->get("col.status"));
        model_->setHeaderData(COL_SCOPE, Qt::Horizontal, langue_->get("col.scope"));
        model_->setHeaderData(COL_PATH, Qt::Horizontal, langue_->get("col.path"));
        model_->setHeaderData(COL_PATH_REF, Qt::Horizontal, langue_->get("col.path_ref"));
    }

    setWindowTitle(langue_->get("app.name"));
    if (model_)
        update_table_all();
}

void MainWindow::load_settings() {
    QSettings settings(QCoreApplication::applicationDirPath() + "/application.ini",
                       QSettings::IniFormat);

    QString lang = settings.value("langue", "").toString();
    if (lang.isEmpty())
        lang = LangueManager::detectSystemLanguage();

    if (!langue_->load(lang)) {
        download_language(lang);
        if (lang != "anglais")
            langue_->load("anglais");
    }

    QByteArray geo = settings.value("geometry").toByteArray();
    if (!geo.isEmpty())
        restoreGeometry(geo);

    save_settings();
}

void MainWindow::save_settings() {
    QSettings settings(QCoreApplication::applicationDirPath() + "/application.ini",
                       QSettings::IniFormat);
    settings.setValue("langue", langue_->currentLanguage());
    settings.setValue("geometry", saveGeometry());
}

void MainWindow::changer_langue(const QString& langCode) {
    if (!langue_->load(langCode))
        download_language(langCode);
    else
        save_settings();
}

void MainWindow::closeEvent(QCloseEvent* event) {
    save_settings();
    QMainWindow::closeEvent(event);
}

void MainWindow::download_language(const QString& code) {
    QString url = QStringLiteral(LANG_BASE_URL) + code + ".txt";
    connect(langue_, &LangueManager::languageDownloaded, this,
        [this, code](const QString& langCode, bool success) {
            if (langCode != code) return;
            if (success && langue_->load(code)) {
                retranslateUi();
                save_settings();
            }
        }, Qt::SingleShotConnection);
    langue_->downloadLanguage(code, url);
}

void MainWindow::show_about() {
    QString title = langue_->get("dialog.about.title");
    QString text = QString(langue_->get("dialog.about.text"))
        .arg(QStringLiteral(APP_NAME))
        .arg(QStringLiteral(APP_VERSION))
        + "<p><a href='" APP_HOMEPAGE_URL "'>"
          APP_HOMEPAGE_URL "</a></p>";
    QMessageBox::about(this, title, text);
}
