#include <QtCore/qfileinfo.h>
#include <QtCore/qdir.h>
#include <QtCore/qprocess.h>
#include <QtCore/qtimer.h>
#include <QtCore/qpointer.h>
#include <QtGui/qdesktopservices.h>
#include <QtGui/qevent.h>
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
#include <QtGui/qscreen.h>
#endif
#include <QtGui/qpainter.h>
#include <QtWidgets/qapplication.h>
#include <QtWidgets/qdesktopwidget.h>
#include <QtWidgets/qmainwindow.h>
#include <QtWidgets/qdialog.h>
#include <QtWidgets/qcompleter.h>
#include <QtWidgets/qcombobox.h>
#include "lafplay_utils.h"

// 除以1000 而不是1024
QString toHumanRead(qint64 size)
{
    if (size < 1000LL) {
        return QString::number(size) + QString::fromUtf8(" B");
    } else if (size < (1000000LL)) {
        return QString::number(1.0 * size / (1000LL), 'f', 2) + QString::fromUtf8(" KB");
    } else if (size < (1000000000LL)) {
        return QString::number(1.0 * size / (1000000LL), 'f', 2) + QString::fromUtf8(" MB");
    } else if (size < (1000000000000LL)) {
        return QString::number(1.0 * size / (1000000000LL), 'f', 2) + QString::fromUtf8(" GB");
    } else if (size < (1000000000000000LL)) {
        return QString::number(1.0 * size / (1000000000000LL), 'f', 2) + QString::fromUtf8(" TB");
    } else if (size < (1000000000000000000LL)) {
        return QString::number(1.0 * size / (1000000000000000LL), 'f', 2) + QString::fromUtf8(" PB");
    } else {
        return QString::fromUtf8("unlimited.");
    }
}

QString toHumanRead(const QDateTime &dt, const QDateTime &now, bool compat)
{
    qint64 delta = (now.toMSecsSinceEpoch() - dt.toMSecsSinceEpoch()) / 1000;
    QString prefix;
    QString suffix;
    QString t;

    if (delta == 0) {
        return "当前";
    }

    if (delta > 0) {
        suffix = "前";
    } else {
        prefix = "后 ";
        delta = -delta;
    }

    if (delta < 60) {
        t = QString::number(delta) + QString::fromUtf8("秒");
    } else if (delta < 60 * 60) {
        qint64 m = delta / 60;
        if (compat) {
            t = QString::fromUtf8("%1 分钟").arg(m);
        } else {
            qint64 s = delta % 60;
            if (s == 0) {
                t = QString::fromUtf8("%1 分钟").arg(m);
            } else {
                t = QString::fromUtf8("%1 分钟 %2 秒").arg(m).arg(s);
            }
        }
    } else if (delta < (60 * 60 * 24)) {
        qint64 h = delta / (60 * 60);
        if (compat) {
            t = QString::fromUtf8("%1 小时").arg(h);
        } else {
            qint64 m = (delta % (60 * 60) / 60);
            if (m == 0) {
                t = QString::fromUtf8("%1 小时").arg(h);
            } else {
                t = QString::fromUtf8("%1 小时 %2 秒").arg(h).arg(m);
            }
        }
    } else if (delta < (60 * 60 * 24 * 30)) {
        qint64 d = delta / (60 * 60 * 24);
        if (compat) {
            t = QString::fromUtf8("%1 天").arg(d);
        } else {
            qint64 h = (delta % (60 * 60 * 24)) / (60 * 60);
            if (h == 0) {
                t = QString::fromUtf8("%1 天").arg(d);
            } else {
                t = QString::fromUtf8("%1 天 %2 小时").arg(d).arg(h);
            }
        }
    } else if (delta < (60 * 60 * 24 * 365)) {
        qint64 m = delta / (60 * 60 * 24 * 30);
        if (compat) {
            t = QString::fromUtf8("%1 月").arg(m);
        } else {
            qint64 d = (delta % (60 * 60 * 24 * 30)) / (60 * 60 * 24);
            if (d == 0) {
                t = QString::fromUtf8("%1 月").arg(m);
            } else {
                t = QString::fromUtf8("%1 月 %2 天").arg(m).arg(d);
            }
        }
    } else {
        qint64 y = delta / (60 * 60 * 24 * 365);
        if (compat) {
            t = QString::fromUtf8("%1 年").arg(y);
        } else {
            qint64 d = (delta % (60 * 60 * 24 * 365)) / (60 * 60 * 24);
            if (d == 0) {
                t = QString::fromUtf8("%1 年").arg(y);
            } else {
                t = QString::fromUtf8("%1 年 %2 天").arg(y).arg(d);
            }
        }
    }
    return prefix + t + suffix;
}

bool CtrlEnterFilter::eventFilter(QObject *obj, QEvent *event)
{
    QWidget *edit = dynamic_cast<QWidget *>(obj);
    if (edit && event->type() == QEvent::KeyPress) {
        QKeyEvent *kEvent = dynamic_cast<QKeyEvent *>(event);
        if (kEvent) {
            bool isEnter = kEvent->key() == Qt::Key_Enter || kEvent->key() == Qt::Key_Return;
            if (isEnter && kEvent->modifiers() == Qt::ControlModifier) {
                kEvent->accept();
                if (parent()) {
                    static_cast<QDialog *>(parent())->accept();
                    return true;
                }
            }
        }
    }
    return QObject::eventFilter(obj, event);
}

void moveToCenter(QWidget * const widget)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QScreen *screen = widget->screen();
#else
    QWidget *screen = QApplication::desktop()->screen();
#endif
    QWidget *parent = widget->parentWidget();
    if (!screen && !parent) {
        qWarning("there must be have either screen or parent.");
        return;
    }
    QRect r = widget->rect();
    if (parent) {
        r.moveCenter(parent->geometry().center());
    } else if (screen) {
        r.moveCenter(screen->geometry().center());
    }
    widget->setGeometry(r);
}

void restorePosition(QSettings &settings, QWidget * const widget, bool center)
{
    QMainWindow *mainWindow = dynamic_cast<QMainWindow *>(widget);
    if (settings.contains("window_geometry")) {
        widget->restoreGeometry(settings.value("window_geometry").toByteArray());
    }
    if (mainWindow && settings.contains("window_state")) {
        mainWindow->restoreState(settings.value("window_state").toByteArray());
    }
    if (center) {
        moveToCenter(widget);
    }
}

void savePosition(QSettings &settings, QWidget * const widget)
{
    QMainWindow *mainWindow = dynamic_cast<QMainWindow *>(widget);
    settings.setValue("window_geometry", widget->saveGeometry());
    if (mainWindow) {
        settings.setValue("window_state", mainWindow->saveState());
    }
}

void retainSizeWhenHidden(QWidget *w)
{
    QSizePolicy policy = w->sizePolicy();
    policy.setRetainSizeWhenHidden(true);
    w->setSizePolicy(policy);
    w->setVisible(false);
}

QIcon toIcon(const QString &normalImg, const QString &hoverImg, const QString &disableImg)
{
    QIcon icon(normalImg);
    icon.addPixmap(QPixmap(hoverImg), QIcon::Active);
    icon.addPixmap(QPixmap(disableImg), QIcon::Disabled);
    return icon;
}

bool openFolderAndSelectedFile(const QString &filePath)
{
    bool success = false;
    // https://stackoverflow.com/questions/3490336/how-to-reveal-in-finder-or-show-in-explorer-with-qt
    const QFileInfo fileInfo(filePath);
#if defined(Q_OS_WIN)
    QString command = "explorer.exe";
    QStringList arguments;
    QString path = QDir::toNativeSeparators(filePath);
    arguments << "/select," << path + "";
    success = QProcess::startDetached(command, arguments);
#elif defined(Q_OS_MAC)
    QProcess::startDetached("/usr/bin/open", { "-R", fileInfo.canonicalFilePath() });
#else
    // we cannot select a file here, because no file browser really supports it...
//    const QString folder = fileInfo.isDir() ? fileInfo.absoluteFilePath() : fileInfo.filePath();
//    const QString app = UnixUtils::fileBrowser(ICore::settings());
//    QStringList browserArgs = ProcessArgs::splitArgs(UnixUtils::substituteFileBrowserParameters(app, folder));
//    QString error;
//    if (browserArgs.isEmpty()) {
//        error = QApplication::translate("Core::Internal", "The command for file browser is not set.");
//    } else {
//        QProcess browserProc;
//        browserProc.setProgram(browserArgs.takeFirst());
//        browserProc.setArguments(browserArgs);
//        const bool success = browserProc.startDetached();
//        error = QString::fromLocal8Bit(browserProc.readAllStandardError());
//        if (!success && error.isEmpty()) {
//            error = QApplication::translate("Core::Internal", "Error while starting file browser.");
//        }
//    }
//    if (!error.isEmpty()) {
//        success = false;
//    }
#endif
    if (!success) {
        success = QDesktopServices::openUrl(QUrl::fromLocalFile(fileInfo.dir().absolutePath()));
    }

    return success;
}

QCompleter *installComboxCompleter(QComboBox *combo)
{
    QCompleter *completer = new QCompleter(combo->model(), combo);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setFilterMode(Qt::MatchContains);
    combo->setCompleter(completer);
    return completer;
}

void resetComboxText(QComboBox *combox, const QStringList &items, bool addCurrentTextIfNotExist)
{
    const QString &text = combox->currentText();
    int index = combox->currentIndex();
    combox->clear();

    bool bFind = false;
    for (int i = 0; i < items.size(); ++i) {
        const QString &item = items.at(i);
        combox->addItem(item);
        if (item == text) {
            combox->setCurrentIndex(i);
            bFind = true;
        }
    }
    if (addCurrentTextIfNotExist && !bFind) {
        if (text.isEmpty()) {
            if (index >= items.size()) {
                index = 0;
            }
            if (index < 0) {
                index = 0;
            }
            combox->setCurrentIndex(index);
        } else {
            if (combox->isEditable()) {
                combox->setCurrentText(text);
            } else {
                combox->addItem(text);
                combox->setCurrentIndex(combox->count() - 1);
            }
        }
    }
}

QByteArray saveWidgetGeometry(QWidget *wSrc)
{
    QWidget *w = wSrc->window();
    if (!w) {
        w = wSrc;
    }
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QScreen *sc = w->screen();
#else
    QScreen *sc = QApplication::screenAt(w->geometry().center());
#endif
    if (!sc) {
        return QByteArray();
    }
    QByteArray encoded;
    QDataStream ds(&encoded, QIODevice::WriteOnly);
    ds << w->geometry() << sc->geometry();
    if (wSrc != w) {
        ds << wSrc->geometry();
    }
    if (ds.status() != QDataStream::Ok) {
        return QByteArray();
    }
    return encoded;
}

void checkGeometry(QWidget *w, const QRect &geometry)
{
    QPointer<QWidget> self(w);
    QTimer::singleShot(0, [self, geometry] {
        if (self.isNull()) {
            return;
        }
        if (self->geometry() == geometry) {
            return;
        }
        QRect rt(geometry);
        if (rt.width() < self->minimumWidth()) {
            rt.setWidth(self->minimumWidth());
        } else if (rt.width() > self->maximumWidth()) {
            rt.setWidth(self->maximumWidth());
        }
        if (rt.height() < self->minimumHeight()) {
            rt.setHeight(self->minimumHeight());
        } else if (rt.width() > self->maximumHeight()) {
            rt.setHeight(self->maximumHeight());
        }
        self->setGeometry(rt);
    });
}

bool restoreWidgetGeometry(QWidget *wSrc, const QByteArray &encoded, bool reCheck)
{
    QWidget *w = wSrc->window();
    if (!w) {
        w = wSrc;
    }
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QScreen *sc = w->screen();
#else
    QScreen *sc = QApplication::screenAt(w->geometry().center());
#endif
    if (!sc) {
        return false;
    }
    QDataStream ds(encoded);
    QRect geometry, oldScGeometry;
    ds >> geometry >> oldScGeometry;
    if (ds.status() != QDataStream::Ok) {
        return false;
    }
    QRect wSrcGeometry;
    ds >> wSrcGeometry;
    QRect realScGeometry = sc->geometry();
    if (realScGeometry == oldScGeometry) {
        w->setGeometry(geometry);
        if (reCheck) {
            checkGeometry(w, geometry);
        }
        return true;
    } else if (realScGeometry.size() == oldScGeometry.size()) {
        geometry.translate(realScGeometry.topLeft() - oldScGeometry.topLeft());
        // 做个防御性编程？
        if (geometry.left() < realScGeometry.left()) {
            geometry.moveLeft(realScGeometry.left());
        } else if (geometry.right() > realScGeometry.right()) {
            geometry.moveRight(realScGeometry.right());
        }
        if (geometry.top() < realScGeometry.top()) {
            geometry.moveTop(realScGeometry.top());
        } else if (geometry.bottom() > realScGeometry.bottom()) {
            geometry.moveBottom(realScGeometry.bottom());
        }

        w->setGeometry(geometry);
        if (reCheck) {
            checkGeometry(w, geometry);
        }
        return true;
    } else {
        return false;
    }
}

WaterMask::WaterMask(const QString &text, bool titled, QWidget *parent)
    : QWidget(parent)
    , text(text)
    , cacheDpr(0)
    , tiled(titled)
{
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setWindowFlags(Qt::WindowStaysOnTopHint | windowFlags());
}

void WaterMask::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    QPainter painter(this);

    QFont font = painter.font();
    if (tiled) {
        double dpr = devicePixelRatioF();
        if (cacheDpr != dpr || cachePixmap.isNull()) {
            cacheDpr = dpr;
            QRect rt(0, 0, 150, 150);
            QPixmap tmpPixmap = QPixmap(rt.width(), rt.height());
            tmpPixmap.fill(Qt::transparent);

            QPainter cachePainter(&tmpPixmap);
            QColor c(Qt::gray);
            c.setAlphaF(0.12);
            cachePainter.setPen(c);

            QTransform matrix;
            matrix.translate(0, 100);
            matrix.rotate(315);
            cachePainter.setTransform(matrix);
            font.setPixelSize(10);
            cachePainter.setFont(font);
            cachePainter.drawText(rt, Qt::AlignHCenter | Qt::AlignTop, text);

            cachePixmap = tmpPixmap.copy(rt);
        }

        painter.drawTiledPixmap(rect(), cachePixmap);
    } else {
        painter.setOpacity(0.2);
        QTransform matrix;
        matrix.translate(20, 100);
        matrix.rotate(315);
        painter.setTransform(matrix);
        font.setPixelSize(28);
        painter.setFont(font);
        painter.drawText(rect(), text);
    }
}

void WaterMask::posist()
{
    QRect rect = parentWidget()->rect();
    if (tiled) {
        setGeometry(rect);
    } else {
        int width = qMin(rect.width(), 150);
        int height = qMin(rect.height(), 150);
        QRect rect2 = QRect(0, 0, width, height);
        rect2.moveBottomRight(rect.bottomRight());
        rect2.moveRight(rect2.right() - 20);
        rect2.moveBottom(rect2.bottom() - 3);
        setGeometry(rect2);
    }
}

ScrollLabel::ScrollLabel(QWidget *parent)
    : QLabel(parent)
    , cacheDpr(0)
    , textWidth(0)
    , textHeight(0)
    , curPos(0)
    , unit(0)
    , tm(-1)
    , pause(false)
    , scroll(false)
{
    startScroll();
}

ScrollLabel::~ScrollLabel()
{
    stopScroll();
}

void ScrollLabel::startScroll()
{
    pause = false;
    if (tm == -1) {
        tm = startTimer(28);
    }
}

void ScrollLabel::stopScroll(bool pause)
{
    this->pause = pause;
    if (tm != -1) {
        killTimer(tm);
        tm = -1;
    }
}

void ScrollLabel::setText(const QString &text, bool scroll)
{
    QLabel::setText(text);
    cacheDpr = 0;
    cachePixmap = QPixmap();
    curPos = 0;
    this->scroll = scroll;

    if (text.isEmpty()) {
        stopScroll();
    } else if (scroll && tm == -1 && isVisible()) {
        startScroll();
    }
}

void ScrollLabel::paintEvent(QPaintEvent *event)
{
    if (!scroll) {
        QLabel::paintEvent(event);
        return;
    }
    double dpr = devicePixelRatioF();
    if (cacheDpr != dpr) {
        //文字的长度由dpi决定
        //在qt源码中,qFont的大小与dpi有关(除非使用Qt::AA_Use96Dpi)
        // QPixmap 的dpi是主屏的dpi
        // QWidget 的dpi是由当前屏幕的dpi决定的
        //所以这里用cachePixmap的方法不太好
        cacheDpr = dpr;

        QFontMetrics metrics = fontMetrics();
        unit = static_cast<int>(metrics.horizontalAdvance("a") * 20 / 100);
        textWidth = static_cast<int>(metrics.horizontalAdvance(text()));
        textHeight = static_cast<int>((metrics.ascent() + metrics.descent()));

        if (qApp->testAttribute(Qt::AA_Use96Dpi)) {
            QRect rtPixmap(0, 0, textWidth, textHeight);
            cachePixmap = QPixmap(rtPixmap.width(), rtPixmap.height());
            QPainter painter(&cachePixmap);
            const QPalette &p = palette();
            painter.setPen(p.color(QPalette::Text));
            painter.setFont(font());
            painter.fillRect(rtPixmap, p.brush(QPalette::Window));
            painter.drawText(rtPixmap, Qt::TextSingleLine | Qt::TextDontClip, text());
        }
    }

    QPainter painter(this);
    if (qApp->testAttribute(Qt::AA_Use96Dpi)) {
        QRect rt(curPos, 0, textWidth, textHeight);
        painter.drawPixmap(rt, cachePixmap);
    } else {
        painter.drawText(rect().adjusted(curPos, 0, 0, 0), Qt::TextSingleLine, text());
    }
}

void ScrollLabel::timerEvent(QTimerEvent *event)
{
    Q_UNUSED(event);
    curPos -= unit;
    if (curPos < -textWidth) {
        curPos = rect().width();
    }
    update();
}

void ScrollLabel::resizeEvent(QResizeEvent *event)
{
    QLabel::resizeEvent(event);
    int w = rect().width();
    if (curPos > w) {
        curPos = w;
    }
}

void ScrollLabel::showEvent(QShowEvent *)
{
    if (scroll && tm == -1) {
        startScroll();
    }
}

void ScrollLabel::hideEvent(QHideEvent *)
{
    if (tm != -1) {
        stopScroll(true);
    }
}

void ScrollLabel::enterEvent(QEvent *)
{
    if (tm != -1) {
        stopScroll(true);
    }
}

void ScrollLabel::leaveEvent(QEvent *)
{
    if (tm == -1 && pause) {
        startScroll();
    }
}

QSize ScrollLabel::sizeHint() const
{
    if (scroll || textHeight == 0) {
        return QLabel::sizeHint();
    }
    return QSize(100, textHeight);
}

QSize ScrollLabel::minimumSizeHint() const
{
    if (scroll || textHeight == 0) {
        return QLabel::sizeHint();
    }
    return QSize(100, textHeight);
}

KeyLineEdit::KeyLineEdit(QWidget *parent /*= nullptr*/)
    : QLineEdit(parent)
{
    setReadOnly(true);
    setPlaceholderText(tr("请按下快捷键"));
}

KeyLineEdit::~KeyLineEdit() { }

void KeyLineEdit::setKey(const QKeySequence &key)
{
    if (key == QKeySequence::UnknownKey) {
        clear();
    } else {
        setText(key.toString());
    }
    this->key = key;
}

QKeySequence KeyLineEdit::getKey() const
{
    return key;
}

void KeyLineEdit::keyPressEvent(QKeyEvent *e)
{
    int uKey = e->key();
    Qt::Key key = static_cast<Qt::Key>(uKey);
    if (key == Qt::Key_unknown) {
        return;
    }
    //除去单个的特殊按键
    if (key == Qt::Key_Control || key == Qt::Key_Shift || key == Qt::Key_Alt || key == Qt::Key_Enter
        || key == Qt::Key_Return || key == Qt::Key_Tab || key == Qt::Key_CapsLock || key == Qt::Key_Escape) {
        return;
    }
    if (key == Qt::Key_Backspace) {
        setKey(QKeySequence::UnknownKey);
        emit keyChanged(QKeySequence::UnknownKey);
        return;
    }
    bool bModifiers = false;
    Qt::KeyboardModifiers modifiers = e->modifiers();
    if (modifiers & Qt::ShiftModifier) {
        uKey += Qt::SHIFT;
        bModifiers = true;
    }
    if (modifiers & Qt::ControlModifier) {
        uKey += Qt::CTRL;
        bModifiers = true;
    }
    if (modifiers & Qt::AltModifier) {
        uKey += Qt::ALT;
        bModifiers = true;
    }

    //不支持单个数字 字母
    bool bNum = (e->key() >= Qt::Key_0 && e->key() <= Qt::Key_9);
    bool bChar = (e->key() >= Qt::Key_A && e->key() <= Qt::Key_Z);
    if ((bNum || bChar) && !bModifiers) {
        // QMessageBox::warning(NULL, QObject::tr("提示"), QObject::tr("快捷键不支持单独数字或者字母!"),
        // QMessageBox::Yes);
        setKey(QKeySequence::UnknownKey);
        emit keyChanged(QKeySequence::UnknownKey);
        return;
    }
    QString qsKey = QKeySequence(uKey).toString();

    //除去Windows常用快捷键
    QStringList blackList;
    blackList << "CTRL+S"
              << "CTRL+C"
              << "CTRL+V"
              << "CTRL+A"
              << "CTRL+D"
              << "CTRL+Z"
              << "CTRL+X";
    if (blackList.contains(qsKey, Qt::CaseInsensitive)) {
        return;
    }
    setKey(QKeySequence(uKey));
    emit keyChanged(uKey);
}
