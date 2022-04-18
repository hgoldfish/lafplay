#include <QtCore/qdebug.h>
#include <QtWidgets/QApplication>
#include <QtWidgets/qmessagebox.h>
#include "lafplay.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    if (argc < 1) {
        QMessageBox::information(nullptr, "LafPlay demo", "No file specified.");
        return 1;
    }
    AnimationViewer player;
    player.resize(1280, 720);
    player.show();
    player.setFile(QString::fromLocal8Bit(argv[1]));
    player.play();
    return app.exec();
}
