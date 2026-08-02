#include <QApplication>
#include <QFont>
#include <QIcon>
#include "AppConfig.hpp"
#include "MainWindow.hpp"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral(APP_NAME));

    QFont f = app.font();
    f.setFamilies({f.family(), QStringLiteral("Noto Color Emoji"), QStringLiteral("Symbola")});
    app.setFont(f);

    QIcon icone;
    icone.addFile(QStringLiteral(":/ico/app-256.png"), QSize(256, 256));
    icone.addFile(QStringLiteral(":/ico/app-128.png"), QSize(128, 128));
    icone.addFile(QStringLiteral(":/ico/app-64.png"), QSize(64, 64));
    icone.addFile(QStringLiteral(":/ico/app-32.png"), QSize(32, 32));
    app.setWindowIcon(icone);

    MainWindow fenetre;
    fenetre.setWindowIcon(icone);
    fenetre.show();

    return app.exec();
}
