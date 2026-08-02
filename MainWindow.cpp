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
#include <QTimer>
#include <QtConcurrent>
#include <QFutureWatcher>
#include <QSortFilterProxyModel>
#include <QStackedWidget>
#include <QScrollArea>
#include <QFrame>
#include <QGroupBox>
#include <QFormLayout>
#include <QFontMetrics>
#include <QResizeEvent>

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

// Proxy de tri + filtres. Le tri numérique est appliqué sur les colonnes de
// version (COL_LOCAL/COL_ONLINE). Le filtrage (statut/catégorie/recherche)
// s'appuie sur la liste d'applications source (même index de ligne).
class SortProxy : public QSortFilterProxyModel {
public:
    explicit SortProxy(QObject* parent = nullptr)
        : QSortFilterProxyModel(parent) {}

    void setAppsSource(QList<AppItem>* apps) { apps_ = apps; }
    void setFilterData(const QString& status, const QString& category, const QString& search) {
        status_ = status;
        category_ = category;
        search_ = search.toLower();
        invalidateFilter();
    }

    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override {
        int col = left.column();
        if (col == COL_LOCAL || col == COL_ONLINE) {
            QString a = left.data(Qt::DisplayRole).toString().trimmed();
            QString b = right.data(Qt::DisplayRole).toString().trimmed();
            QVersionNumber va = QVersionNumber::fromString(a);
            QVersionNumber vb = QVersionNumber::fromString(b);
            if (!va.isNull() && !vb.isNull())
                return va < vb;
            return QString::compare(a, b, Qt::CaseInsensitive) < 0;
        }
        return QString::compare(left.data().toString(),
                                right.data().toString(),
                                Qt::CaseInsensitive) < 0;
    }

    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override {
        if (!apps_ || sourceRow >= apps_->size())
            return true;
        const AppItem& app = apps_->at(sourceRow);
        if (!status_.isEmpty() && app.statusKey() != status_)
            return false;
        if (!category_.isEmpty() && app.category != category_)
            return false;
        if (!search_.isEmpty() &&
            !app.name.toLower().contains(search_) &&
            !app.id.toLower().contains(search_))
            return false;
        return true;
    }

private:
    QList<AppItem>* apps_ = nullptr;
    QString status_;
    QString category_;
    QString search_;
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

    // Analyse différée : la fenêtre s'affiche immédiatement, la détection
    // se lance ensuite sans bloquer l'interface.
    QTimer::singleShot(0, this, &MainWindow::analyze_all);
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
    layout->setContentsMargins(0, 0, 0, 0);

    // Onglets par catégorie (sélecteur)
    tabs_ = new QTabWidget;
    tabs_->setDocumentMode(true);
    layout->addWidget(tabs_);

    // Stack : page applications + page réglages
    stack_ = new QStackedWidget;
    layout->addWidget(stack_, 1);

    // ── Page Applications ──────────────────────────────────────────────
    apps_page_ = new QWidget;
    auto* apps_layout = new QVBoxLayout(apps_page_);
    apps_layout->setContentsMargins(4, 4, 4, 4);

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

    apps_layout->addLayout(filter_bar);

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
    proxy_ = new SortProxy(this);
    static_cast<SortProxy*>(proxy_)->setAppsSource(&apps_);
    proxy_->setSourceModel(model_);
    table_->setModel(proxy_);
    table_->setSortingEnabled(true);
    table_->sortByColumn(COL_NAME, Qt::AscendingOrder);
    table_->setWordWrap(true);
    table_->setTextElideMode(Qt::ElideNone);
    table_->horizontalHeader()->setStretchLastSection(false);
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    table_->horizontalHeader()->setMinimumSectionSize(40);
    table_->horizontalHeader()->setMaximumSectionSize(900);
    // La colonne Chemin s'adapte au contenu ET absorbe l'espace restant
    table_->horizontalHeader()->setSectionResizeMode(COL_PATH, QHeaderView::Interactive);
    // Hauteur des lignes adaptée au texte multiligne (retour à la ligne)
    table_->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    connect(model_, &QStandardItemModel::itemChanged,
            this, &MainWindow::on_item_checked);
    connect(table_, &QTableView::clicked, this, &MainWindow::open_homepage);

    apps_layout->addWidget(table_, 1);

    // Barre de progression
    progress_bar_ = new QProgressBar;
    progress_bar_->setRange(0, 100);
    progress_bar_->setValue(0);
    progress_bar_->setVisible(false);
    apps_layout->addWidget(progress_bar_);

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

    apps_layout->addWidget(phase2);

    connect(cb_download_, &QCheckBox::checkStateChanged, this, &MainWindow::selection_changed);
    connect(cb_command_, &QCheckBox::checkStateChanged, this, &MainWindow::selection_changed);
    connect(cb_install_, &QCheckBox::checkStateChanged, this, &MainWindow::selection_changed);
    connect(cb_repair_, &QCheckBox::checkStateChanged, this, &MainWindow::selection_changed);
    connect(btn_actions_, &QPushButton::clicked, this, &MainWindow::apply_action_phase2);
    connect(btn_repair_, &QPushButton::clicked, this, &MainWindow::repair_incorrect_install);

    status_label_ = new QLabel;
    statusBar()->addWidget(status_label_, 1);

    // ── Page Réglages : liste verticale de groupes, un par application ──
    auto* settings_widget = new QWidget;
    settings_widget_ = settings_widget;
    auto* sl = new QVBoxLayout(settings_widget);
    sl->setContentsMargins(4, 4, 4, 4);

    settings_scroll_ = new QScrollArea;
    settings_scroll_->setWidgetResizable(true);
    settings_scroll_->setFrameShape(QFrame::NoFrame);
    settings_list_ = new QWidget;
    auto* list_layout = new QVBoxLayout(settings_list_);
    list_layout->setContentsMargins(4, 4, 4, 4);
    list_layout->setSpacing(8);
    settings_list_->setLayout(list_layout);
    settings_scroll_->setWidget(settings_list_);
    sl->addWidget(settings_scroll_, 1);

    auto* sbar = new QHBoxLayout;
    btn_save_settings_ = new QPushButton;
    btn_reset_settings_ = new QPushButton;
    sbar->addWidget(btn_save_settings_);
    sbar->addWidget(btn_reset_settings_);
    sbar->addStretch(1);
    sl->addLayout(sbar);
    connect(btn_save_settings_, &QPushButton::clicked,
            this, &MainWindow::save_commands_ini);
    connect(btn_reset_settings_, &QPushButton::clicked,
            this, &MainWindow::reset_commands_defaults);
    settings_widget->setObjectName(QStringLiteral("reglages"));

    // Ajout des deux pages au stack
    stack_->addWidget(apps_page_);
    stack_->addWidget(settings_widget_);

    // Sélection de page selon l'onglet
    connect(tabs_, &QTabWidget::currentChanged, this, [this]() {
        int idx = tabs_->currentIndex();
        QString obj = idx >= 0 && tabs_->widget(idx)
            ? tabs_->widget(idx)->objectName() : QString();
        if (obj == QStringLiteral("reglages"))
            stack_->setCurrentIndex(1);
        else
            stack_->setCurrentIndex(0);
        filter_changed();
    });

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

    // Ajustement des largeurs au contenu
    table_->resizeColumnsToContents();
    table_->horizontalHeader()->setSectionResizeMode(COL_ICON, QHeaderView::Fixed);
    table_->horizontalHeader()->setSectionResizeMode(COL_SELECT, QHeaderView::Fixed);
    table_->setColumnWidth(COL_ICON, 30);
    table_->setColumnWidth(COL_SELECT, 30);
    // Largeur minimale pour les colonnes principales lisibles
    table_->horizontalHeader()->setSectionResizeMode(COL_NAME, QHeaderView::Interactive);
    table_->setColumnWidth(COL_NAME, 170);
    table_->setColumnWidth(COL_LOCAL, 100);
    table_->setColumnWidth(COL_ONLINE, 100);
    table_->setColumnWidth(COL_STATUS, 150);
    table_->setColumnWidth(COL_SCOPE, 90);
    table_->setColumnWidth(COL_PATH_REF, 70);
    // La colonne Chemin : largeur = au moins celle du contenu, sinon remplit
    table_->setColumnWidth(COL_PATH, path_column_width());

    apply_filter();
}

// Largeur de la colonne Chemin : au moins celle du texte le plus long,
// mais au plus la largeur restante de la fenêtre.
int MainWindow::path_column_width() const {
    int maxContent = 100;
    for (const auto& app : apps_) {
        QFontMetrics fm(table_->font());
        maxContent = qMax(maxContent, fm.horizontalAdvance(app.install_path) + 20);
    }
    int viewport = table_->viewport()->width();
    int other = 0;
    for (int c = 0; c < COL_COUNT; ++c) {
        if (c != COL_PATH)
            other += table_->columnWidth(c);
    }
    return qMin(qMax(maxContent, 250), qMax(viewport - other, 250));
}

void MainWindow::update_row(int row) {
    if (row < 0 || row >= apps_.size())
        return;
    const AppItem& app = apps_[row];

    QStandardItem* ck = model_->item(row, COL_SELECT);
    if (ck) ck->setCheckState(app.is_selected ? Qt::Checked : Qt::Unchecked);

    // Icône de l'application : icône locale (icones_app) > exe installé > générique
    if (auto* icon = model_->item(row, COL_ICON)) {
        QIcon ic;
        // 1. Icône locale fournie dans icones_app/<id>.png
        QString localIcon = QCoreApplication::applicationDirPath()
            + "/icones_app/" + app.id + ".png";
        if (QFile::exists(localIcon)) {
            ic = QIcon(localIcon);
        } else if (app.installed && !app.install_path.isEmpty()) {
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
    if (!proxy_) return;

    int stIdx = filter_status_->currentIndex();
    QString stData = filter_status_->itemData(stIdx).toString();
    QString catData = tabs_ ? tabs_->currentIndex() > 0
                                ? tabs_->widget(tabs_->currentIndex())->objectName()
                                : QString()
                            : QString();
    QString search = search_box_->text();

    auto* sp = static_cast<SortProxy*>(proxy_);
    sp->setFilterData(stData, catData, search);
}

void MainWindow::rebuild_tabs() {
    if (!tabs_) return;

    // Sauvegarde de la catégorie courante
    QString current = tabs_->currentIndex() > 0
        ? tabs_->widget(tabs_->currentIndex())->objectName() : QString();

    while (tabs_->count() > 0)
        tabs_->removeTab(0);

    auto* allTab = new QWidget;
    allTab->setObjectName(QString());
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
    // Onglet Réglages : widget vide, la page réelle est affichée via le stack
    auto* reglagesTab = new QWidget;
    reglagesTab->setObjectName(QStringLiteral("reglages"));
    tabs_->addTab(reglagesTab, langue_->get("tab.reglages"));
    tabs_->setCurrentIndex(restore);
    connect(tabs_, &QTabWidget::currentChanged, this, &MainWindow::filter_changed,
            Qt::UniqueConnection);
}

void MainWindow::analyze_all() {
    if (checker_->isChecking()) {
        set_status_message(langue_->get("status.checking"));
        return;
    }
    if (detect_watcher_ && detect_watcher_->isRunning())
        return;

    // Chargement du manifeste
    apps_ = AppItem::loadManifest(apps_path_);
    if (apps_.isEmpty()) {
        QMessageBox::warning(this, langue_->get("app.name"),
            langue_->get("status.manifest_missing") + "\n" + apps_path_);
        return;
    }

    // Application des surcharges utilisateur (commandes.ini) si présent,
    // sinon génère le fichier par défaut (éditable par l'utilisateur).
    QString iniPath = QCoreApplication::applicationDirPath() + "/commandes.ini";
    if (!QFile::exists(iniPath))
        AppItem::writeIniFile(apps_, iniPath);
    else
        AppItem::applyIniOverrides(apps_, iniPath);

    // Détection locale en arrière-plan (parallèle, non bloquant)
    progress_bar_->setVisible(true);
    progress_bar_->setRange(0, 100);
    progress_bar_->setValue(0);
    set_status_message(langue_->get("status.detecting"));

    AppDetector* det = detector_;
    auto future = QtConcurrent::map(apps_, [det](AppItem& item) {
        det->detect(item);
    });

    detect_watcher_ = new QFutureWatcher<void>(this);
    connect(detect_watcher_, &QFutureWatcher<void>::finished,
            this, &MainWindow::on_detection_done);
    detect_watcher_->setFuture(future);
}

void MainWindow::on_detection_done() {
    detect_watcher_->deleteLater();
    detect_watcher_ = nullptr;

    progress_bar_->setRange(0, apps_.size());
    progress_bar_->setValue(apps_.size());

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
    populate_settings_tab();

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
    // L'index vient de la vue (proxy) : mapper vers la source
    QModelIndex src = proxy_ ? proxy_->mapToSource(index) : index;
    if (src.row() < 0 || src.row() >= apps_.size())
        return;
    const AppItem& app = apps_[src.row()];
    if (!app.homepage.isEmpty())
        QDesktopServices::openUrl(QUrl(app.homepage));
}

void MainWindow::on_item_checked(QStandardItem* item) {
    if (!item || item->column() != COL_SELECT)
        return;
    int srcRow = item->row();
    if (srcRow < 0 || srcRow >= apps_.size())
        return;
    apps_[srcRow].is_selected = (item->checkState() == Qt::Checked);
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
    // Collecte des apps sélectionnées (qu'elles soient installées ou non :
    // on peut télécharger/installer une app non encore installée).
    QList<AppItem*> selected;
    for (int row = 0; row < apps_.size(); ++row) {
        if (apps_[row].is_selected)
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
    // Priorité 1 : réparer les applications cochées par l'utilisateur.
    QList<AppItem*> selected;
    for (int row = 0; row < apps_.size(); ++row) {
        if (apps_[row].is_selected)
            selected.append(&apps_[row]);
    }

    QStringList names;
    for (auto* app : selected)
        names << app->name;

    // Priorité 2 : si rien n'est coché, détection automatique des
    // installations suspectes (installé mais PAS référencé dans le PATH,
    // mauvais emplacement, vestige, ou version locale inconnue).
    if (selected.isEmpty()) {
        for (int row = 0; row < apps_.size(); ++row) {
            AppItem& app = apps_[row];
            bool suspect = app.installed &&
                           ((!app.referenced_in_path && !app.detectCommands().isEmpty()) ||
                            app.local_version.isEmpty() && !app.versionLocalRegex.isEmpty() &&
                                app.versionLocalRegistry.isEmpty());
            if (suspect) {
                selected.append(&app);
                names << app.name;
            }
        }
    }

    if (selected.isEmpty()) {
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

    for (auto* app : selected)
        app->is_selected = true;

    set_status_message(langue_->get("status.repairing"));
    installer_->repair(selected);
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

void MainWindow::populate_settings_tab() {
    if (!settings_list_)
        return;

    // Vidage de la liste précédente
    QLayout* old = settings_list_->layout();
    if (old) {
        while (QLayoutItem* it = old->takeAt(0)) {
            if (QWidget* w = it->widget())
                w->deleteLater();
            delete it;
        }
    }
    settings_edits_.clear();

    auto* list_layout = settings_list_->layout();
    if (!list_layout) {
        list_layout = new QVBoxLayout(settings_list_);
        list_layout->setContentsMargins(4, 4, 4, 4);
        list_layout->setSpacing(8);
        settings_list_->setLayout(list_layout);
    }

    for (int r = 0; r < apps_.size(); ++r) {
        const AppItem& app = apps_[r];

        auto* box = new QGroupBox;
        box->setObjectName(QStringLiteral("group_") + app.id);
        auto* vbox = new QVBoxLayout(box);
        vbox->setContentsMargins(8, 6, 8, 6);

        // En-tête : icône + nom
        auto* head = new QHBoxLayout;
        auto* icon_label = new QLabel;
        QString localIcon = QCoreApplication::applicationDirPath()
            + "/icones_app/" + app.id + ".png";
        QIcon ic;
        if (QFile::exists(localIcon))
            ic = QIcon(localIcon);
        else
            ic = style()->standardIcon(QStyle::SP_ComputerIcon);
        icon_label->setPixmap(ic.pixmap(20, 20));
        auto* name_label = new QLabel(app.name);
        QFont f = name_label->font();
        f.setBold(true);
        name_label->setFont(f);
        head->addWidget(icon_label);
        head->addWidget(name_label);
        head->addStretch(1);
        vbox->addLayout(head);

        auto* form = new QFormLayout;
        form->setLabelAlignment(Qt::AlignRight);
        form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

        QList<QLineEdit*> edits;
        QLineEdit* e1 = new QLineEdit(app.updateCommand.value("windows").toString());
        QLineEdit* e2 = new QLineEdit(app.updateCommand.value("linux").toString());
        QLineEdit* e3 = new QLineEdit(app.repairCommand.value("windows").toString());
        QLineEdit* e4 = new QLineEdit(app.repairCommand.value("linux").toString());
        QLineEdit* e5 = new QLineEdit(app.downloadUrl);
        edits << e1 << e2 << e3 << e4 << e5;

        // Rangées : une ligne par paramètre
        form->addRow(langue_->get("settings.update_windows"), e1);
        form->addRow(langue_->get("settings.update_linux"), e2);
        form->addRow(langue_->get("settings.repair_windows"), e3);
        form->addRow(langue_->get("settings.repair_linux"), e4);
        form->addRow(langue_->get("settings.download"), e5);

        vbox->addLayout(form);
        box->setLayout(vbox);
        list_layout->addWidget(box);

        // Séparateur visuel entre deux applications
        if (r < apps_.size() - 1) {
            auto* sep = new QFrame;
            sep->setFrameShape(QFrame::HLine);
            sep->setFrameShadow(QFrame::Sunken);
            sep->setStyleSheet("color: #888;");
            list_layout->addWidget(sep);
        }

        settings_edits_.append(edits);
    }

    auto* vlist = qobject_cast<QVBoxLayout*>(list_layout);
    if (vlist)
        vlist->addStretch(1);
}

void MainWindow::save_commands_ini() {
    if (apps_.isEmpty())
        return;
    // Récupère les valeurs éditables depuis la liste de groupes
    for (int r = 0; r < apps_.size() && r < settings_edits_.size(); ++r) {
        const QList<QLineEdit*>& edits = settings_edits_[r];
        if (edits.size() < 5)
            continue;
        auto& app = apps_[r];
        app.updateCommand["windows"] = edits[0]->text();
        app.updateCommand["linux"] = edits[1]->text();
        app.repairCommand["windows"] = edits[2]->text();
        app.repairCommand["linux"] = edits[3]->text();
        app.downloadUrl = edits[4]->text();
    }
    AppItem::writeIniFile(apps_,
        QCoreApplication::applicationDirPath() + "/commandes.ini");
    set_status_message(langue_->get("status.settings_saved"));
}

void MainWindow::reset_commands_defaults() {
    // Recharge les valeurs par défaut depuis apps.json
    apps_ = AppItem::loadManifest(apps_path_);
    AppItem::applyIniOverrides(apps_,
        QCoreApplication::applicationDirPath() + "/commandes.ini");
    populate_settings_tab();
    update_table_all();
    set_status_message(langue_->get("status.settings_reset"));
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

    // Réglages
    if (btn_save_settings_) {
        btn_save_settings_->setText(langue_->get("settings.save"));
        btn_reset_settings_->setText(langue_->get("settings.reset"));
    }
    // Re-traduction des labels des groupes de réglages
    if (settings_list_) {
        const QStringList labelKeys = {
            QStringLiteral("settings.update_windows"),
            QStringLiteral("settings.update_linux"),
            QStringLiteral("settings.repair_windows"),
            QStringLiteral("settings.repair_linux"),
            QStringLiteral("settings.download")
        };
        const auto boxes = settings_list_->findChildren<QGroupBox*>();
        for (QGroupBox* box : boxes) {
            // Le QFormLayout est imbriqué dans le layout vertical du groupe
            auto* vbox = qobject_cast<QVBoxLayout*>(box->layout());
            QFormLayout* form = nullptr;
            if (vbox) {
                for (int i = 0; i < vbox->count(); ++i) {
                    if (auto* fl = qobject_cast<QFormLayout*>(vbox->itemAt(i)->layout())) {
                        form = fl;
                        break;
                    }
                }
            }
            if (form) {
                for (int i = 0; i < form->rowCount(); ++i) {
                    QLayoutItem* li = form->itemAt(i, QFormLayout::LabelRole);
                    if (li && i < labelKeys.size()) {
                        if (auto* lbl = qobject_cast<QLabel*>(li->widget()))
                            lbl->setText(langue_->get(labelKeys.at(i)));
                    }
                }
            }
        }
    }

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

void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    // Réajuste la colonne Chemin pour qu'elle remplisse la fenêtre
    if (table_ && table_->model() && apps_.size() > 0)
        table_->setColumnWidth(COL_PATH, path_column_width());
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
