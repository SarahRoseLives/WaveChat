#include <QApplication>
#include <QFile>
#include <QFont>
#include "ui/MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("WaveChat");
    app.setOrganizationName("WaveChat");

    QFont font("Segoe UI", 10);
    font.setStyleStrategy(QFont::PreferAntialias);
    app.setFont(font);

    QFile styleFile(":/style.qss");
    if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
        app.setStyleSheet(styleFile.readAll());
        styleFile.close();
    }

    MainWindow window;
    window.show();

    return app.exec();
}
