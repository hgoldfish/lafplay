#include <QPainter>
#include <QPixmapCache>
#include <QHeaderView>
#include <QPushButton>
#include <QVBoxLayout>
#include <QStyleFactory>
#include <QTextDocument>
#include <QTextObject>
#include <QScrollBar>
#include <QRandomGenerator>
#include <QAbstractTextDocumentLayout>
#include <QtCore/qloggingcategory.h>
#include <QApplication>
#include <QToolTip>
#include <QScreen>
#include <QGraphicsSceneMouseEvent>
#include "locust_table_widget.h"
#include "QtWaitingSpinner/waitingspinnerwidget.h"

#define ITEM_HEIGHT_NOEXPAND 126
#define IMAGE_SIZE 119
#define ITEM_TOP_TABLE_HEIGHT 36
#define ITEM_NORMAL_SPACE 2
#define IMAGE_SPACE 4
#define HEADERVIEW_HEIGHT 36
#define SCROLLBAR_WIDTH 6
#define TABLEPLAN_ROW_HEIGHT 21
#define TABLEPLAN_ROW_WHAT_SHOW_DEFAULT 4
#define TABLEPLAN_ROW_WHAT_SHOW_MAX 50
#define BTN_EXPAND_SIZE 30
#define COLOR_WIDGET_WIDTH 11
#define TABLE_FIRST_COLUMN_WIDTH 490
#define TABLE_DEFAULT_COLOUMN_WIDTH 85
#define MORE_ITEM_HEIGHT 50
#define TABLEPLAN_FIRST_COLOUMN_WIDTH 100
#define TABLEPLAN_COLOUMN_SMALL_WIDTH 60
#define TABLEPLAN_FIRST_COLOUMN_LEFT_SIZE_MORE 30
#define TEXT_BLOCK_LINE_HEIGHT 12
#define LABEL_NUM_WIDTH 50
#define NOTES_WIDTH 50
#define FRAME_WIDTH 1

LocustTableWidgetBaseHeaderView::LocustTableWidgetBaseHeaderView(QWidget *parent)
    : QHeaderView(Qt::Horizontal, parent)
    , bgColor(Qt::white)
    , firstColmnBgColor(Qt::white)
    , borderColor(QColor("#8FA9C2"))
    , textLeftFirstSection(false)
{

}

QJsonObject LocustTableWidgetBaseHeaderView::saveBaseSectionsSize() const
{
    QJsonObject obj;
    QAbstractItemModel *baseTableModel = model();
    if (!baseTableModel) {
        return obj;
    }
    int colCount = baseTableModel->columnCount();
    for (int i = 0; i < colCount; i++) {
        const QString &text = baseTableModel->headerData(i, Qt::Horizontal,
            Qt::DisplayRole).toString();
        int w = sectionSize(i);
        obj.insert(text, w);
    }
    return obj;
}

void LocustTableWidgetBaseHeaderView::restoreBaseSectionSize(const QJsonObject &obj)
{
    QAbstractItemModel *baseTableModel = model();
    if (!baseTableModel) {
        return;
    }
    int colCount = baseTableModel->columnCount();
    for (int i = 0; i < colCount; i++) {
        const QString &text = baseTableModel->headerData(i, Qt::Horizontal,
            Qt::DisplayRole).toString();
        if (obj.contains(text)) {
            int w = obj.value(text).toInt();
            if (w > 0) {
                resizeSection(i, w);
            }
        }
    }
}

void LocustTableWidgetBaseHeaderView::_paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const
{
    if (!rect.isValid()) {
        return;
    }
    QAbstractItemModel *baseTableModel = model();
    if (!baseTableModel) {
        return;
    }
    QStyleOptionHeader opt;
    initStyleOption(&opt);

    opt.rect = rect;
    opt.text = baseTableModel->headerData(logicalIndex, Qt::Horizontal,
        Qt::DisplayRole).toString();

    if (isSortIndicatorShown() && sortIndicatorSection() == logicalIndex)
        opt.sortIndicator = (sortIndicatorOrder() == Qt::AscendingOrder)
        ? QStyleOptionHeader::SortDown : QStyleOptionHeader::SortUp;

    // setup the style options structure
    opt.section = logicalIndex;
    //    QVariant textAlignment = baseTableModel->headerData(logicalIndex, Qt::Horizontal,
    //        Qt::TextAlignmentRole);
    //    opt.textAlignment = Qt::Alignment(textAlignment.isValid()
    //        ? Qt::Alignment(textAlignment.toInt())
    //        : defaultAlignment());

#if 0
    style()->drawControl(QStyle::CE_HeaderSection, &opt, painter, this);
#else
    if (logicalIndex == 0) {
        painter->setBrush(firstColmnBgColor);
    } else {
        painter->setBrush(bgColor);
    }    
    painter->setPen(borderColor);
    QRectF rt(opt.rect);
    painter->drawRect(rt.adjusted(-0.5, 0, -0.5, 0));
#endif

    QRegion clipRegion = painter->clipRegion();
    painter->setClipRect(opt.rect);

    QStyleOptionHeader subopt = opt;
    subopt.rect = style()->subElementRect(QStyle::SE_HeaderLabel, &opt, this);
    if (subopt.rect.isValid()) {
        QFont f("黑体");
        f.setPointSize(9);
        painter->setFont(f);

        if (logicalIndex == 0 && firstColumnColor.isValid()) {
            painter->setPen(firstColumnColor.value<QColor>());
        } else {
            painter->setPen(QColor("#51606E"));
        }

        if (textLeftFirstSection && logicalIndex == 0) {
            painter->drawText(subopt.rect.adjusted(20, 0, 0, 0), Qt::AlignVCenter | Qt::AlignLeft | Qt::TextDontClip, subopt.text);
        } else {
            painter->drawText(subopt.rect, Qt::AlignCenter | Qt::TextDontClip, subopt.text);
        }
    }
    if (opt.sortIndicator != QStyleOptionHeader::None) {
        subopt.rect = style()->subElementRect(QStyle::SE_HeaderArrow, &opt, this);
        style()->drawPrimitive(QStyle::PE_IndicatorHeaderArrow, &subopt, painter, this);
    }
    painter->setClipRegion(clipRegion);
}

void LocustTableWidgetBaseHeaderView::paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const
{
    _paintSection(painter, rect, logicalIndex);
}

void LocustTableWidgetBaseHeaderView::showEvent(QShowEvent *event)
{
    QHeaderView::showEvent(event);
    emit showedChanged();
}

void LocustTableWidgetBaseHeaderView::hideEvent(QHideEvent *event)
{
    QHeaderView::hideEvent(event);
    emit showedChanged();
}

void initBaseTable(LocustTableWidgetBaseHeaderView *baseHeaderView, QAbstractTableModel *baseTableModel, int firstSectionDefaultSize, int &baseWidthHint)
{
    baseWidthHint = 0;
    baseHeaderView->setModel(baseTableModel);
    if (baseTableModel) {
        baseHeaderView->setStretchLastSection(false);
        int subColCount = baseTableModel->columnCount();
        for (int i = 0; i < subColCount; i++) {
            baseWidthHint += baseHeaderView->sectionSize(i);
        }
    }
}

LocustTableWidgetHeaderView::LocustTableWidgetHeaderView(QAbstractItemModel *model, LocustTableWidget *tableWidget, QWidget *parent /*= nullptr*/)
    : LocustTableWidgetBaseHeaderView(parent)
    , tableWidget(tableWidget)
    , baseHeaderView(new LocustTableWidgetBaseHeaderView(this))
    , userWidget(new QWidget(this))
    , baseWidthHint(0)
{
    userWidget->setVisible(false);

    setModel(model);
    setContextMenuPolicy(Qt::CustomContextMenu);

    int colCount = model->columnCount();

    //setCascadingSectionResizes(true);
    setMinimumSectionSize(20);

    setDefaultSectionSize(TABLE_DEFAULT_COLOUMN_WIDTH);
    widthHint = colCount * defaultSectionSize();

    resizeSection(0, tableWidget->firstColumnDefaultWidth());
    widthHint += tableWidget->firstColumnDefaultWidth() - defaultSectionSize();

    setMaximumHeight(HEADERVIEW_HEIGHT);
    setMinimumWidth(widthHint - defaultSectionSize());


    int flexColumn = colCount - 1;
    setSectionResizeMode(flexColumn, QHeaderView::Stretch);

    baseHeaderView->firstColumnColor = QColor("#003233");
    //baseHeaderView->setCascadingSectionResizes(true);
    baseHeaderView->setMinimumSectionSize(20);
    baseHeaderView->setContextMenuPolicy(Qt::CustomContextMenu);

    QString tableName;
    baseTableModel = tableWidget->createBaseModel(&tableName, true);
    initBaseTable(baseHeaderView, baseTableModel, tableWidget->baseTableFirstSectionDefaultSize(), baseWidthHint);
    curBaseTableName = tableName;
    connect(baseHeaderView, &QHeaderView::sectionResized, this, &LocustTableWidgetHeaderView::_baseSectionResized);
    connect(tableWidget->hBar, &QScrollBar::valueChanged, baseHeaderView, &LocustTableWidgetHeaderView::setOffset);

    makeBeauty();
    baseHeaderView->textLeftFirstSection = tableWidget->baseTableFirstSectionTextLeftAlign();
    connect(this, &QHeaderView::sectionResized, this, &LocustTableWidgetHeaderView::_sectionResized);
    connect(baseHeaderView, &LocustTableWidgetBaseHeaderView::showedChanged, this, &LocustTableWidgetHeaderView::baseHeaderSizeChanged);
}

LocustTableWidgetHeaderView::~LocustTableWidgetHeaderView()
{
    delete baseTableModel;
    delete baseHeaderView;
    delete userWidget;
}

QSize LocustTableWidgetHeaderView::sizeHint() const
{
    return QSize(widthHint, HEADERVIEW_HEIGHT);
}

int LocustTableWidgetHeaderView::getBaseHeaderWidthHint() const
{
    if (!baseHeaderView->isVisible()) {
        return 0;
    }
    return baseWidthHint;
}

int LocustTableWidgetHeaderView::getBaseHeaderWidth() const
{
    if (!baseHeaderView->isVisible()) {
        return 0;
    }
    return baseHeaderView->width();
}

void LocustTableWidgetHeaderView::switchBaseTable()
{
    QJsonObject baseTableSectionSize = property("baseTableSectionSize").toJsonObject();
    QString lastTableName(curBaseTableName);
    baseTableSectionSize[lastTableName] = baseHeaderView->saveBaseSectionsSize();
    setProperty("baseTableSectionSize", baseTableSectionSize);

    disconnect(baseHeaderView, &QHeaderView::sectionResized, this, &LocustTableWidgetHeaderView::_baseSectionResized);
    QString tableName;
    QAbstractTableModel *baseTableModel = tableWidget->createBaseModel(&tableName, true);
    initBaseTable(baseHeaderView, baseTableModel, tableWidget->baseTableFirstSectionDefaultSize(), baseWidthHint);
    curBaseTableName = tableName;
    connect(baseHeaderView, &QHeaderView::sectionResized, this, &LocustTableWidgetHeaderView::_baseSectionResized);

    if (baseTableSectionSize.contains(tableName)) {
        const QJsonObject &lastSectionSize = baseTableSectionSize.value(tableName).toObject();
        baseHeaderView->restoreBaseSectionSize(lastSectionSize);
    }

    if (this->baseTableModel) {
        delete this->baseTableModel;
    }
    this->baseTableModel = baseTableModel;

    if (baseTableModel) {
        if (!baseHeaderView->isVisible()) {
            baseHeaderView->setVisible(true);
        }
        if (userWidget->isVisible()) {
            userWidget->setVisible(false);
        }
        baseHeaderView->textLeftFirstSection = tableWidget->baseTableFirstSectionTextLeftAlign();
    } else {
        if (baseHeaderView->isVisible()) {
            baseHeaderView->setVisible(false);
        }
        userWidget->setVisible(tableWidget->initUserTitleWidget(userWidget));
    }
    tableWidget->hBarRangeOrPageChange();
    updateSubHeaderViewGeometry();
}

LocustTableWidgetBaseHeaderView *LocustTableWidgetHeaderView::getBaseHeaderView() const
{
    return baseHeaderView;
}

QJsonObject LocustTableWidgetHeaderView::saveSectionsSize() const
{
    QJsonObject obj(saveBaseSectionsSize());
    QJsonObject baseTableSectionSize = property("baseTableSectionSize").toJsonObject();
    const QString &lastTableName = curBaseTableName;
    baseTableSectionSize[lastTableName] = baseHeaderView->saveBaseSectionsSize();
    obj.insert("baseTableSectionSize", baseTableSectionSize);
    return obj;
}

void LocustTableWidgetHeaderView::restoreSectionSize(const QJsonObject &obj)
{
    restoreBaseSectionSize(obj);
    if (obj.contains("baseTableSectionSize")) {
        const QJsonObject &baseTableSectionSize = obj.value("baseTableSectionSize").toObject();
        setProperty("baseTableSectionSize", baseTableSectionSize);

        const QString &tableName = curBaseTableName;
        if (baseTableSectionSize.contains(tableName)) {
            const QJsonObject &lastSectionSize = baseTableSectionSize.value(tableName).toObject();
            baseHeaderView->restoreBaseSectionSize(lastSectionSize);
        }
    }
}

void LocustTableWidgetHeaderView::setBaseHeaderViewBgColor(const QColor &c)
{
    baseHeaderView->bgColor = baseHeaderView->firstColmnBgColor = c;
}


void LocustTableWidgetHeaderView::reInitSectionsSize()
{
    setDefaultSectionSize(TABLE_DEFAULT_COLOUMN_WIDTH);
    resizeSection(0, tableWidget->firstColumnDefaultWidth());

    if (baseTableModel) {
        int defaultSectionSize = baseHeaderView->defaultSectionSize();
        int colCount = baseTableModel->columnCount();
        int w = baseHeaderView->width();
        if (colCount > 0 && w > 0) {
            defaultSectionSize = w / colCount;
        }
        if (defaultSectionSize > 120) {
            defaultSectionSize = 120;
        } else if (defaultSectionSize < 70) {
            defaultSectionSize = 70;
        }
        baseHeaderView->setDefaultSectionSize(defaultSectionSize);
    }

    disconnect(baseHeaderView, &QHeaderView::sectionResized, this, &LocustTableWidgetHeaderView::_baseSectionResized);
    initBaseTable(baseHeaderView, baseTableModel, tableWidget->baseTableFirstSectionDefaultSize(), baseWidthHint);
    connect(baseHeaderView, &QHeaderView::sectionResized, this, &LocustTableWidgetHeaderView::_baseSectionResized);
}

void LocustTableWidgetHeaderView::_sectionResized(int logicalIndex, int oldSize, int newSize)
{
    widthHint += newSize - oldSize;
    if (logicalIndex == 0) {
        updateSubHeaderViewGeometry();
    }
}

void LocustTableWidgetHeaderView::_baseSectionResized(int, int oldSize, int newSize)
{
    baseWidthHint += newSize - oldSize;
}

void LocustTableWidgetHeaderView::updateSubHeaderViewGeometry()
{
    int left = sectionSize(0);
    int fixLeft = left;
    int fixTop = tableWidget->mainHeaderViewHeight();
    int w = width() - fixLeft - FRAME_WIDTH;
    int h = height() - tableWidget->mainHeaderViewHeight() - FRAME_WIDTH;
    baseHeaderView->setGeometry(fixLeft, fixTop, w, h);
    userWidget->setGeometry(fixLeft, fixTop, w, h);

    emit baseHeaderSizeChanged();
}

void LocustTableWidgetHeaderView::makeBeauty()
{
    //样式
    setFrameShape(QFrame::NoFrame);
    bgColor = QColor("#E2F0FF");
    //baseHeaderView->firstColmnBgColor = baseHeaderView->bgColor = qRgb(241, 245, 249);
}

void LocustTableWidgetHeaderView::paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const
{
    QRect rt;
    if (logicalIndex > 0) {
        rt = QRect(rect.left(), rect.top(), rect.width(), tableWidget->mainHeaderViewHeight());
    } else {
        rt = rect;
    }
    _paintSection(painter, rt, logicalIndex);
}

void LocustTableWidgetHeaderView::resizeEvent(QResizeEvent *event)
{
    QHeaderView::resizeEvent(event);
    updateSubHeaderViewGeometry();

    emit sizeChanged(width());
}


void LocustTableWidgetHeaderView::paintEvent(QPaintEvent *event)
{
    LocustTableWidgetBaseHeaderView::paintEvent(event);

    QRect rt = rect().adjusted(0, 0, 0, 0);
    QPainter painter(viewport());
    painter.setPen(borderColor);
    painter.drawLine(rt.bottomLeft(), rt.bottomRight());
}

LocustTableWidgetItemView::LocustTableWidgetItemView(LocustTableWidget *tableWidget, int logicIndex, int visualIndex, bool expand, QWidget *parent)
    : QWidget(parent)
    , logicIndex(logicIndex)
    , visualIndex(visualIndex)
    , tableWidget(tableWidget)
    , expand(expand)
    , baseTable(new LocustBaseTableView(this))
    , baseTableModel(tableWidget->createBaseModel(nullptr, false))
    , baseTableItemDelegate(tableWidget->createBaseItemDelegate())
    , lbHover(new QLabel(baseTable))
    , lbNum(new QLabel(this))
    , btnAccept(new QPushButton(baseTable))
    , btnRefuse(new QPushButton(baseTable))
    , btnExpand(new QPushButton(this))
    , docLeft(new QTextDocument())
    , docRight(new QTextDocument())
    , dataAddressLeft(true)
    , userWidget(new QWidget(this))
{
    baseTable->setObjectName("baseTable");
    btnExpand->setObjectName("btnExpand");
    lbNum->setObjectName("lbNum");
    btnRefuse->setObjectName("btnRefuse");
    btnAccept->setObjectName("btnAccept");

    userWidget->setVisible(false);

    QPalette palette;
    palette.setColor(QPalette::Window, QColor(255, 255, 255, 0));
    setPalette(palette);

    setContextMenuPolicy(Qt::CustomContextMenu);
    lbHover->setVisible(false);
    lbHover->setText(">");
    lbHover->setAttribute(Qt::WA_TransparentForMouseEvents);
    QPalette p;
    p.setColor(QPalette::WindowText, QColor("#003233"));
    lbHover->setPalette(p);

    initBaseTable();

    initBtns();

    btnExpand->setCheckable(true);
    initExpand();
    expandChanged();

    docLeft->setUndoRedoEnabled(false);
    docRight->setUndoRedoEnabled(false);

    lbNum->setText(QString::number(visualIndex + 1));
    lbNum->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    lbNum->setVisible(false);

    makeBeauty();
    LocustTableWidgetHeaderView *headerview = tableWidget->getHeaderview();
    connect(headerview, &QHeaderView::sectionResized, this, &LocustTableWidgetItemView::headerViewSectionResized);
    connect(btnExpand, &QPushButton::toggled, this, &LocustTableWidgetItemView::expandItem);
    connect(baseTable, &QTableView::activated, this, &LocustTableWidgetItemView::modifyBaseTable);
    connect(baseTable, &QTableView::customContextMenuRequested, this, &LocustTableWidgetItemView::openBaseTableCustomMenu);
    connect(baseTable, &QAbstractItemView::entered, this, &LocustTableWidgetItemView::baseTableEntered);
    connect(baseTable, &LocustBaseTableView::mouseLeaved, this, [this] {
        lbHover->setVisible(false);
        btnAccept->setVisible(false);
        btnRefuse->setVisible(false);
    });
    connect(this, &QWidget::customContextMenuRequested, this, &LocustTableWidgetItemView::openCustomMenu);
    connect(headerview->getBaseHeaderView(), &QHeaderView::sectionResized, this, &LocustTableWidgetItemView::subHeaderViewSectionResized);
}

LocustTableWidgetItemView::~LocustTableWidgetItemView()
{
    for (QWidget *w: userDefineWidgets) {
        delete w;
    }
    if (userWidget) {
        delete userWidget;
    }
    if (baseTableItemDelegate) {
        delete baseTableItemDelegate;
    }
    delete docLeft;
    delete docRight;
    delete btnExpand;
    delete btnRefuse;
    delete btnAccept;
    delete lbHover;
    delete lbNum;
    delete baseTableModel;
    delete baseTable;
}

void LocustTableWidgetItemView::dataChanged(int logicIndex, int heightBefore)
{
    this->logicIndex = logicIndex;

    resetBaseTable();
    updateTableGeometry();

    resetExpand(heightBefore);
    update();
}

bool LocustTableWidgetItemView::changeExpand()
{
    if (expand) {
        btnExpand->setChecked(false);
        //expandItem(false);
        return true;
    }
    if (!btnExpand->isVisible()) {
        return false;
    }
    btnExpand->setChecked(true);
    //expandItem(true);
    return true;
}

bool LocustTableWidgetItemView::isExpand()
{
    return expand;
}

bool LocustTableWidgetItemView::mousePress(const QPoint &pt, QMouseEvent *parentEvent)
{
    if (parentEvent->button() == Qt::LeftButton) {
        QPoint pos(pt);
        const QRect &dataRt = dataAddressLeft ? docRtLeft : docRtRight;
        if (dataRt.contains(pos)) {
            pos -= dataRt.topLeft();
            QTextDocument *doc = dataAddressLeft ? docLeft : docRight;
            const QString &url = doc->documentLayout()->anchorAt(pos);
            if (!url.isEmpty()) {
                emit openLink(url);
                return true;
            }
        }
    }
    return false;
}

bool LocustTableWidgetItemView::mouseRelease(const QPoint &pt, QMouseEvent *parentEvent)
{
    return false;
}

bool LocustTableWidgetItemView::mouseDoubleClick(const QPoint &pt, QMouseEvent *parentEvent)
{
    bool onDoubleClick = property("onDoubleClick").toBool();
    if (!onDoubleClick) {
        setProperty("onDoubleClick", true);
        emit openMainItem(logicIndex);
        setProperty("onDoubleClick", false);
        return true;
    }
    return false;
}

bool LocustTableWidgetItemView::mouseMove(const QPoint &pt, QGraphicsSceneMouseEvent *parentEvent)
{
    if (docRtLeft.contains(pt)) {
        QAbstractTableModel *mainModel = tableWidget->mainModel;
        if (mainModel) {
            const QModelIndex &detailIndex = mainModel->index(logicIndex, 0);
            const QString &detail = detailIndex.data(LocustTableWidget::DetailRole).toString();
            if (!detail.isEmpty()) {
                QToolTip::showText(parentEvent->screenPos(), detail, this, docRtLeft);
            }
        }
    } else if (docRtRight.contains(pt)) {
        QAbstractTableModel *mainModel = tableWidget->mainModel;
        if (mainModel) {
            const QModelIndex &detailIndex = mainModel->index(logicIndex, 0);
            const QString &detail = detailIndex.data(LocustTableWidget::DetailSecondRole).toString();
            if (!detail.isEmpty()) {
                const QString &detailBottom = detailIndex.data(LocustTableWidget::DetailSecondBottomRole).toString();
                QToolTip::showText(parentEvent->screenPos(), QString("%1\n%2").arg(detail).arg(detailBottom), this, docRtRight);
            }
        }
    } else {
        QRect iconRect(IMAGE_SPACE, IMAGE_SPACE, tableWidget->iconSize(), tableWidget->iconSize());
        if (iconRect.contains(pt)) {
            QRect rt(parentEvent->screenPos() + iconRect.topLeft() - pt, iconRect.size());
            emit iconHovered(logicIndex, rt);
        }
    }
    return false;
}

void LocustTableWidgetItemView::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    updateTableGeometry();
    updateBtnGeometry();
    updateUserWidgetGeometry();
    updateUserDefineWidgetGeometry();

    lbNum->setGeometry(0, 0, LABEL_NUM_WIDTH, 20);
}

void LocustTableWidgetItemView::paintEvent(QPaintEvent *)
{
    //QWidget::paintEvent(event);
    QPainter painter(this);

    //写内容
    LocustTableWidgetHeaderView *headerview = tableWidget->getHeaderview();
    if (!headerview) {
        return;
    }
    QAbstractItemModel *model = tableWidget->mainModel;
    if (!model) {
        return;
    }

    QFont font = painter.font();
    font.setPointSize(tableWidget->fontPointSize());
    painter.setFont(font);

    QColor gray(QColor("#8FA9C2"));
    QColor fontColor(QColor("#51606E"));

    bool isSelect = tableWidget->isItemSelected(logicIndex);
    painter.setPen(gray);
    
    int w = headerview->sectionSize(0);

    QRect firtRt(0, 0, w, tableWidget->defaultItemHeight());
    QRect secondRt(firtRt.topRight(), rect().bottomRight());
    painter.drawRect(secondRt.adjusted(0, 0, 0, -1));
    painter.drawRect(firtRt.adjusted(0, 0, -1, -1));
    if (expand) {
        painter.fillRect(firtRt, QColor("#E2F0FF"));

        QRect rt = secondRt.adjusted(0, /*ITEM_TOP_TABLE_HEIGHT*/0, 0, 0);
        QLinearGradient gradient(rt.topLeft(), rt.bottomLeft());
        gradient.setColorAt(0, QColor("#D5EAFF"));
        gradient.setColorAt(1, QColor("#FDFDFD"));
        painter.setBrush(gradient);
        painter.drawRect(rt.adjusted(0, 0, 0, -1));
        painter.setBrush(Qt::NoBrush);
    }

    const QModelIndex &detailIndex = model->index(logicIndex, 0);
    QRect imageLeft(firtRt.topLeft(), QSize(IMAGE_SPACE, firtRt.height()));
    if (isSelect) {
        painter.fillRect(imageLeft, QColor("#4885ff"));
    } else {
        painter.fillRect(imageLeft, QColor("#DFE0E4"));
    }
    QRect iconRect(IMAGE_SPACE, IMAGE_SPACE, tableWidget->iconSize(), tableWidget->iconSize());
    painter.drawPixmap(iconRect, detailIndex.data(LocustTableWidget::FaceRole).value<QPixmap>());

    const QVariant &stop = detailIndex.data(LocustTableWidget::SpecialNoteRole);
    if (stop.isValid()) {
        const QPoint &topRight = iconRect.topRight();
#define STOP_IMAGE_WIDTH 32
#define STOP_IMAGE_Height 20
        painter.drawPixmap(QRect(topRight.x() - STOP_IMAGE_WIDTH, topRight.y(), STOP_IMAGE_WIDTH, STOP_IMAGE_Height),
            stop.value<QPixmap>());
    }

    QFontMetrics fm = fontMetrics();
    if (w > tableWidget->iconSize()) {
        const QVariant &notes = detailIndex.data(LocustTableWidget::NotesRole);
        if (notes.isValid()) {
            const QString &s = notes.toString();

            QRect fRt = fm.boundingRect(s);

            QRect rt(firtRt.topRight(), QSize(fRt.width(), 20));
            rt.translate(-rt.width(), 0);

            painter.fillRect(rt, QColor("#FD7B3D"));
            painter.setPen(Qt::white);
            painter.drawText(rt, Qt::AlignRight | Qt::AlignVCenter, s);
        }

        QRect r(iconRect.topRight(), QPoint(w, firtRt.height()));
        QRegion clipRegion = painter.clipRegion();
        painter.save();
        painter.setClipRect(r);
        
        const QString &detail2 = detailIndex.data(LocustTableWidget::DetailSecondRole).toString();
        
        if (!detail2.isEmpty()) {
            docRtLeft = QRect(r.topLeft(), QSize(r.width() / 2 + TABLEPLAN_FIRST_COLOUMN_LEFT_SIZE_MORE, r.height()));
            docRtRight = QRect(docRtLeft.topRight(), QSize(r.width() - docRtLeft.width(), r.height()));

            const QString &dataAddress = detailIndex.data(LocustTableWidget::DataAddressRole).toString();
            const QString &bottom = detailIndex.data(LocustTableWidget::DetailSecondBottomRole).toString();
            paintDetail(detail2, dataAddress, bottom, docRtRight.adjusted(ITEM_NORMAL_SPACE, -2, -2, -2), painter, fontColor, docRight);

            const QString &detail = detailIndex.data(LocustTableWidget::DetailRole).toString();
            if (!detail.isEmpty()) {
                paintDetail(detail, QString(), QString(), docRtLeft.adjusted(ITEM_NORMAL_SPACE, -2, -2, -2), painter, fontColor, docLeft);
            }
            if (dataAddressLeft) {
                dataAddressLeft = false;
            }
        } else {
            docRtLeft = r;

            const QString &dataAddress = detailIndex.data(LocustTableWidget::DataAddressRole).toString();
            const QString &detail = detailIndex.data(LocustTableWidget::DetailRole).toString();
            if (!detail.isEmpty()) {
                paintDetail(detail, dataAddress, QString(), docRtLeft.adjusted(ITEM_NORMAL_SPACE, -2, -2, -2), painter, fontColor, docLeft);
            }

            if (!dataAddressLeft) {
                dataAddressLeft = true;
            }
        }
        painter.setClipRegion(clipRegion);
        painter.restore();
    }

    if (!baseTable->isVisible() && (!userWidget || !userWidget->isVisible()) && !tableWidget->tipsWhenBaseTableUnvisible.isEmpty()) {
        painter.setPen(fontColor);
        QRect rt = baseTable->geometry();
        rt.setHeight(height() - rt.top());
        painter.drawText(rt, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextDontClip, tableWidget->tipsWhenBaseTableUnvisible);
    }

    painter.setBrush(QColor("#E2F0FF"));

    if (tableWidget->mainHeaderViewHeight() > 0) {
        int columnCount = model->columnCount();
        bool baseHeaderVisible = baseTableModel != nullptr;
        for (int i = 1; i < columnCount; i++) {
            int secionSize = getSectionSize(i);
            QRectF rt(w, 0, secionSize, baseHeaderVisible ? ITEM_TOP_TABLE_HEIGHT : tableWidget->defaultItemHeight());
            w += secionSize;

            painter.setPen(gray);
            painter.drawRect(rt.adjusted(-0.5, 0, -0.5, 0));

            if (i < columnCount - 1) {
                painter.drawLine(rt.topRight(), rt.bottomRight());
            }

            QVariant data = model->data(model->index(logicIndex, i), Qt::DisplayRole);
            if (!data.isValid()) {
                continue;
            }

            painter.setPen(fontColor);

            const QString &text = data.toString();
            const QString elidedText = fm.elidedText(text, Qt::ElideRight, rt.width());
            painter.drawText(rt, Qt::AlignCenter | Qt::TextSingleLine | Qt::TextDontClip, elidedText);
        }
    }
}

void LocustTableWidgetItemView::leaveEvent(QEvent *event)
{
    lbNum->setVisible(false);
}

void LocustTableWidgetItemView::enterEvent(QEvent *event)
{
    lbNum->setVisible(true);
}

int LocustTableWidgetItemView::getSectionSize(int visualIndex)
{
    return tableWidget->getHeaderview()->sectionSize(visualIndex);
}

void LocustTableWidgetItemView::updateTableGeometry()
{
    int topHeight;
    if (tableWidget->mainHeaderViewHeight() > 0) {
        topHeight = ITEM_TOP_TABLE_HEIGHT + ITEM_NORMAL_SPACE + 5;
    } else {
        topHeight = 0;
    }
    int secionSize = getSectionSize(0);

    int rowCount = baseTableModel ? baseTableModel->rowCount(QModelIndex()) : 0;
    int h = expand ? baseTable->verticalHeader()->defaultSectionSize() * qMin(rowCount, TABLEPLAN_ROW_WHAT_SHOW_MAX) : baseTable->verticalHeader()->defaultSectionSize() * qMin(tableWidget->defaultBaseRowWantShow(), rowCount);
    baseTable->setGeometry(secionSize, topHeight,
        width() - secionSize - FRAME_WIDTH, h + baseTable->lineWidth());
}

void LocustTableWidgetItemView::updateUserWidgetGeometry()
{
    if (userWidget) {
        int topHeight;
        if (tableWidget->mainHeaderViewHeight() > 0) {
            topHeight = ITEM_TOP_TABLE_HEIGHT;
        } else {
            topHeight = 0;
        }

        int secionSize = getSectionSize(0);
        userWidget->setGeometry(secionSize, topHeight + FRAME_WIDTH, width() - secionSize - FRAME_WIDTH, height() - topHeight - FRAME_WIDTH * 2);
    }
}

void LocustTableWidgetItemView::updateUserDefineWidgetGeometry()
{
    int columnCount = tableWidget->mainModel->columnCount();
    int column = 0;
    int left = 0;
    bool baseHeaderVisible = baseTableModel != nullptr;
    // 从左到右的顺序
    for (QWidget *w : userDefineWidgets) {
        int dstColumn = w->property("column").toInt();
        for (int i = column; i < columnCount; i++) {
            if (dstColumn == i) {
                break;
            }
            int secionSize = getSectionSize(i);
            left += secionSize;
        }
        if (dstColumn == columnCount) {
            break;
        }
        w->setGeometry(left, 0, getSectionSize(dstColumn), baseHeaderVisible ? ITEM_TOP_TABLE_HEIGHT : tableWidget->defaultItemHeight());
    }
}

void LocustTableWidgetItemView::updateBtnGeometry()
{
    btnExpand->setGeometry(width() - BTN_EXPAND_SIZE, height() - BTN_EXPAND_SIZE, BTN_EXPAND_SIZE, BTN_EXPAND_SIZE);
}

void LocustTableWidgetItemView::makeBeauty()
{
    if (baseTableItemDelegate) {
        baseTable->setShowGrid(false);
        baseTable->setItemDelegate(baseTableItemDelegate);
    }
    btnExpand->setStyleSheet("QPushButton:!checked{border-image:url(res:btn_expand.png);background:transparent;}\n"
        "QPushButton:checked{border-image:url(res:btn_no_expand.png);background:transparent;}");
    lbNum->setStyleSheet(QString("QLabel{background-color: %1; color:#ffffff}\n").arg(QColor("#E3544C").name()));
    btnAccept->setStyleSheet("QPushButton{border-image:url(res:btn_accept.png);background:none;}");
    btnRefuse->setStyleSheet("QPushButton{border-image:url(res:btn_refuse.png);background:none;}");
}

void LocustTableWidgetItemView::acceptBtnClicked()
{
    int logicsBaseTable = btnAccept->property("logics").toInt();
    emit acceptBaseTable(logicIndex, logicsBaseTable, true);
}

void LocustTableWidgetItemView::refuseBtnClicked()
{
    int logicsBaseTable = btnAccept->property("logics").toInt();
    emit acceptBaseTable(logicIndex, logicsBaseTable, false);
}

void LocustTableWidgetItemView::headerViewSectionResized(int logicalIndex, int oldSize, int newSize)
{
    Q_UNUSED(oldSize);
    Q_UNUSED(newSize);
    if (logicalIndex == 0) {
        updateTableGeometry();
        updateUserWidgetGeometry();
    }
    updateUserDefineWidgetGeometry();
    update();
}

void LocustTableWidgetItemView::subHeaderViewSectionResized(int logicalIndex, int oldSize, int newSize)
{
    baseTable->horizontalHeader()->resizeSection(logicalIndex, newSize);
}

void LocustTableWidgetItemView::expandItem(bool expand)
{
    if (expand == this->expand) {
        return;
    }
    this->expand = expand;
    if (expand) {
        btnExpand->setToolTip(tr("点击收起"));
    } else {
        btnExpand->setToolTip(tr("点击展开"));
    }
    expandChanged();
    int h = tableWidget->defaultItemHeight();
    if (expand) {
        int rowCount = baseTableModel ? baseTableModel->rowCount(QModelIndex()) : 0;
        h += baseTable->frameWidth() * 2 + baseTable->verticalHeader()->defaultSectionSize() * (qMin(TABLEPLAN_ROW_WHAT_SHOW_MAX, qMax(tableWidget->defaultBaseRowWantShow(), rowCount)) - tableWidget->defaultBaseRowWantShow());
    }

    emit heightChanged(visualIndex, h);
}

void LocustTableWidgetItemView::modifyBaseTable(const QModelIndex &index)
{
    emit openBaseTable(logicIndex, index.data(LocustTableWidget::SrcIndexRole).toInt());
}

void LocustTableWidgetItemView::openBaseTableCustomMenu(const QPoint &pos)
{
    Q_UNUSED(pos);
    const QModelIndex &index = baseTable->currentIndex();
    if (!index.isValid()) {
        return;
    }
    emit showBaseTableCustomMenu(logicIndex, index.data(LocustTableWidget::SrcIndexRole).toInt());
}

void LocustTableWidgetItemView::openCustomMenu(const QPoint &pos)
{
    Q_UNUSED(pos);
    emit showCustomMenu(logicIndex);
}

void LocustTableWidgetItemView::baseTableEntered(const QModelIndex &index)
{
    if (tableWidget->enableBaseTableHoverItem()) {
        lbHover->setVisible(true);
    }
    int row = index.row();
    int defaultRowHeight = baseTable->verticalHeader()->defaultSectionSize();
    int top = defaultRowHeight * row;
    lbHover->setGeometry(5, top, defaultRowHeight, defaultRowHeight);

    if (index.data(LocustTableWidget::ShowAcceptRefuseBtnRole).toBool()) {
        top += 2;
        int r = baseTable->width() - defaultRowHeight;
        btnRefuse->setVisible(true);
        btnRefuse->move(r, top);

        r -= defaultRowHeight;
        btnAccept->setVisible(true);
        btnAccept->move(r, top);

        btnAccept->setProperty("logics", index.data(LocustTableWidget::SrcIndexRole).toInt());
        btnRefuse->setProperty("logics", index.data(LocustTableWidget::SrcIndexRole).toInt());
    } else {
        btnRefuse->setVisible(false);
        btnAccept->setVisible(false);
    }

    const QPoint &pt = baseTable->mapToGlobal(QPoint(0, 0));
    emit baseTableHovered(logicIndex, index.data(LocustTableWidget::SrcIndexRole).toInt(), QRect(pt, QSize(baseTable->width(), defaultRowHeight)));
}

void LocustTableWidgetItemView::initExpand()
{
    int rowCount = baseTableModel ? baseTableModel->rowCount(QModelIndex()) : 0;
    if (rowCount <= tableWidget->defaultBaseRowWantShow()) {
        btnExpand->setVisible(false);
        expand = false;
    } else {
        btnExpand->setVisible(true);
        btnExpand->setChecked(expand);
        if (expand) {
            btnExpand->setToolTip(tr("点击收起"));
        } else {
            btnExpand->setToolTip(tr("点击展开"));
        }
    }
}

void LocustTableWidgetItemView::resetExpand(int heightBefore)
{
    initExpand();
    expandChanged();

    int h;
    if (expand) {
        int rowCount = baseTableModel ? baseTableModel->rowCount(QModelIndex()) : 0;
        h = tableWidget->defaultItemHeight();
        h += baseTable->frameWidth() * 2 + baseTable->verticalHeader()->defaultSectionSize() * (qMin(TABLEPLAN_ROW_WHAT_SHOW_MAX, qMax(tableWidget->defaultBaseRowWantShow(), rowCount)) - tableWidget->defaultBaseRowWantShow());
    } else {
        h = tableWidget->defaultItemHeight();
    }
    if (h != heightBefore) {
        emit heightChanged(visualIndex, h);
    }
}

void LocustTableWidgetItemView::initBtns()
{
    btnAccept->setVisible(false);
    btnRefuse->setVisible(false);
    int rowHeight = baseTable->verticalHeader()->defaultSectionSize();
    btnAccept->resize(rowHeight - 4, rowHeight - 4);
    btnRefuse->resize(rowHeight - 4, rowHeight - 4);
    connect(btnRefuse, &QPushButton::clicked, this, &LocustTableWidgetItemView::refuseBtnClicked);
    connect(btnAccept, &QPushButton::clicked, this, &LocustTableWidgetItemView::acceptBtnClicked);
}

QString LocustTableWidgetItemView::makeDetailHtml(const QString &detail, const QString &dataAddress, const QString &detail2, const QColor &fontColor)
{
    QStringList sl;
    {
        QString pa("<span style=\"white-space:pre-line;color:#%1;\">%2</span>");
        sl.append(pa.arg(QString::number(fontColor.rgb(), 16), detail.toHtmlEscaped()));
    }
    if (!dataAddress.isEmpty()) {
        QString pa("<span style=\"white-space:pre-line;color:%1;\">%2</span><a href=\"%3\"><span style=\" text-decoration: underline; color:%4;\">打开</span></a>");
        sl.append(pa.arg(fontColor.name()).arg(tr("\n数据包地址:")).arg(dataAddress).arg(QColor("#6C5CCC").name()));
    }
    if (!detail2.isEmpty())
    {
        QString pa("<span style=\"white-space:pre-line;color:#%1;\">\n%2</span>");
        sl.append(pa.arg(QString::number(fontColor.rgb(), 16), detail2.toHtmlEscaped()));
    }

    return QString("<p>%1</p>").arg(sl.join(QString::fromUtf8("\n")));
}

void LocustTableWidgetItemView::paintDetail(const QString &detail, const QString &dataAddress, const QString &detail2, const QRect &rtDetail, QPainter &painter, const QColor &fontColor, QTextDocument *doc)
{
    painter.translate(rtDetail.topLeft());
    QFont fontDetail = painter.font();
    //fix qt-bugs: qt would find this->screen() what is not right
    if (fontDetail.pointSize() != -1) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
        QScreen *screen = tableWidget->screen();
#else
        QScreen *screen = QApplication::screenAt(tableWidget->mapToGlobal(tableWidget->geometry().center()));
#endif
        int px = 12;
        if (screen) {
            px = screen->logicalDotsPerInch() * fontDetail.pointSize() / 72;
        }
        fontDetail.setPixelSize(px);
    }    

    QTextOption op;
    op.setAlignment(Qt::AlignLeft | Qt::AlignTop);
    op.setWrapMode(QTextOption::WrapAnywhere);
    doc->setDefaultFont(fontDetail);
    doc->setTextWidth(rtDetail.width());
    doc->setDefaultTextOption(op);
    doc->setHtml(makeDetailHtml(detail, dataAddress, detail2, fontColor));
    doc->drawContents(&painter);

    painter.translate(QPoint(0, 0) - rtDetail.topLeft());
}

void LocustTableWidgetItemView::initBaseTable()
{
    const LocustTableWidgetBaseHeaderView *baseHeadView = tableWidget->getBaseHeaderview();
    tableWidget->resetBaseModel(baseTableModel, logicIndex);
    baseTable->setModel(baseTableModel);
    baseTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    baseTable->setSelectionMode(QAbstractItemView::SingleSelection);
    baseTable->setContextMenuPolicy(Qt::CustomContextMenu);
    baseTable->horizontalHeader()->setStretchLastSection(false);
    baseTable->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    if (baseTableModel) {
        int columnCount = baseTableModel->columnCount(QModelIndex());
        for (int i = 0; i < columnCount; i++) {
            baseTable->horizontalHeader()->setSectionResizeMode(i, baseHeadView->sectionResizeMode(i));
            baseTable->horizontalHeader()->resizeSection(i, baseHeadView->sectionSize(i));
        }
    }

    baseTable->horizontalHeader()->setVisible(false);
    baseTable->verticalHeader()->setMinimumSectionSize(tableWidget->baseTableDefaultRowHeight());
    baseTable->verticalHeader()->setDefaultSectionSize(tableWidget->baseTableDefaultRowHeight());
    baseTable->verticalHeader()->setVisible(false);
    baseTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    baseTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    baseTable->setFocusPolicy(Qt::NoFocus);
    baseTable->setAutoScroll(false);

    baseTable->horizontalScrollBar()->setValue(tableWidget->hBar->value());
    connect(tableWidget->hBar, &QScrollBar::valueChanged, baseTable->horizontalScrollBar(), &QScrollBar::setValue);

    baseTable->setMouseTracking(true);

    if (baseTableModel) {
        int rowCount = baseTableModel->rowCount(QModelIndex());
        if (rowCount == 0) {
            baseTable->setVisible(false);
        }
    } else {
        baseTable->setVisible(false);
        if (userWidget) {
            userWidget->setVisible(tableWidget->initUserWidget(logicIndex, userWidget, true));
        }
    }

    if (!userDefineWidgets.isEmpty()) {
        for (QWidget *w : userDefineWidgets) {
            delete w;
        }
        userDefineWidgets.clear();
    }
    const QList<int> &columns = tableWidget->mainModel->headerData(logicIndex, Qt::Horizontal, LocustTableWidget::UserDefineWidgetColumnListRole).value<QList<int>>();
    if (!columns.isEmpty()) {
        for (int column : columns) {
            const QModelIndex &index = tableWidget->mainModel->index(logicIndex, column);
            QWidget *w = index.data(LocustTableWidget::UserDefineWidgetRole).value<QWidget *>();
            if (!w) {
                continue;
            }
            w->setParent(this);
            w->setProperty("column", column);
            userDefineWidgets.append(w);
        }
    }
}

void LocustTableWidgetItemView::resetBaseTable()
{
    tableWidget->resetBaseModel(baseTableModel, logicIndex);
    int rowCount = baseTableModel ? baseTableModel->rowCount(QModelIndex()) : 0;
    if (rowCount > 0) {
        if (!baseTable->isVisible()) {
            baseTable->setVisible(true);
        }
    } else {
        if (baseTable->isVisible()) {
            baseTable->setVisible(false);
        }
    }
    if (!baseTableModel) {       
        if (userWidget) {
            userWidget->setVisible(tableWidget->initUserWidget(logicIndex, userWidget, true));
        }
    }

    if (!userDefineWidgets.isEmpty()) {
        for (QWidget *w : userDefineWidgets) {
            delete w;
        }
        userDefineWidgets.clear();
    }
    const QList<int> &columns = tableWidget->mainModel->headerData(logicIndex, Qt::Horizontal, LocustTableWidget::UserDefineWidgetColumnListRole).value<QList<int>>();
    if (!columns.isEmpty()) {
        for (int column : columns) {
            const QModelIndex &index = tableWidget->mainModel->index(logicIndex, column);
            QWidget *w = index.data(LocustTableWidget::UserDefineWidgetRole).value<QWidget *>();
            if (!w) {
                continue;
            }
            w->setParent(this);
            w->setProperty("column", column);
            userDefineWidgets.append(w);
        }
    }
}

void LocustTableWidgetItemView::expandChanged()
{
    if (expand) {
        baseTable->setStyleSheet("QTableView{border:2px solid #8FA9C2;}\n");
    } else {
        baseTable->setStyleSheet("QTableView{border:none;}\n");
    }
    if (baseTableModel) {
        baseTableModel->setHeaderData(0, Qt::Horizontal, expand, LocustTableWidget::ExpandRole);
    }
}

LocustTableWidgetItem::LocustTableWidgetItem(LocustTableWidget *tableWidget)
    : tableWidget(tableWidget)
    , listItem(nullptr)
    , proxyItem(nullptr)
    , height(tableWidget->defaultItemHeight())
    , leftTop(0)
{
}

LocustTableWidgetItem::~LocustTableWidgetItem()
{
    hideWidget();
}

void LocustTableWidgetItem::heightChanged(int visualIndex, int height)
{
    if (!proxyItem) {
        return;
    }
    int delta = height - this->height;
    QRectF rt = proxyItem->geometry();
    proxyItem->setGeometry(QRectF(rt.topLeft(), QSizeF(rt.width(), height)));
    if (delta > 0) {
        proxyItem->setZValue(0x3f3f3f3f);
    } else {
        proxyItem->setZValue(visualIndex);
    }
    emit expandChanged(visualIndex, delta);
}

void LocustTableWidgetItem::headviewSizeChanged(int width)
{
    if (proxyItem) {
        QRectF rt = proxyItem->geometry();
        proxyItem->setGeometry(QRectF(rt.topLeft(), QSizeF(width, height)));
    }
}

void LocustTableWidgetItem::resetPos(int leftTop)
{
    this->leftTop = leftTop;
    if (proxyItem) {
        proxyItem->setGeometry(QRect(0, leftTop, tableWidget->headerview->width(), height));
    }
}

void LocustTableWidgetItem::resetLogicIndex(int logicIndex)
{
    if (listItem) {
        listItem->dataChanged(logicIndex, height);
    }
}

void LocustTableWidgetItem::showWidget(int logicIndex, int visualIndex)
{
    if (!proxyItem) {
        proxyItem = new QGraphicsProxyWidget();
        tableWidget->scene->addItem(proxyItem);
        listItem = new LocustTableWidgetItemView(tableWidget, logicIndex, visualIndex, height > tableWidget->defaultItemHeight());
        listItem->setAcceptDrops(true);
        listItem->installEventFilter(tableWidget);
        proxyItem->setWidget(listItem);
        proxyItem->setZValue(visualIndex);

        proxyItem->setGeometry(QRect(0, leftTop, tableWidget->headerview->width(), height));
        connect(tableWidget->headerview, &LocustTableWidgetHeaderView::sizeChanged, this, &LocustTableWidgetItem::headviewSizeChanged);
        connect(listItem, &LocustTableWidgetItemView::heightChanged, this, &LocustTableWidgetItem::heightChanged);
        connect(listItem, &LocustTableWidgetItemView::openBaseTable, tableWidget, &LocustTableWidget::openBaseTable, Qt::QueuedConnection);
        connect(listItem, &LocustTableWidgetItemView::openMainItem, tableWidget, &LocustTableWidget::openMainItem, Qt::QueuedConnection);
        connect(listItem, &LocustTableWidgetItemView::showBaseTableCustomMenu, tableWidget, &LocustTableWidget::showBaseTableCustomMenu, Qt::QueuedConnection);
        connect(listItem, &LocustTableWidgetItemView::showCustomMenu, tableWidget, &LocustTableWidget::showCustomMenu, Qt::QueuedConnection);
        connect(listItem, &LocustTableWidgetItemView::baseTableHovered, tableWidget, &LocustTableWidget::baseTableHovered, Qt::QueuedConnection);
        connect(listItem, &LocustTableWidgetItemView::iconHovered, tableWidget, &LocustTableWidget::mainItemIconHovered, Qt::QueuedConnection);
        connect(listItem, &LocustTableWidgetItemView::acceptBaseTable, tableWidget, &LocustTableWidget::acceptBaseTable, Qt::QueuedConnection);
        connect(listItem, &LocustTableWidgetItemView::openLink, tableWidget, &LocustTableWidget::openLink, Qt::QueuedConnection);

        connect(this, &LocustTableWidgetItem::itemShowed, tableWidget, &LocustTableWidget::itemShowed);
        emit itemShowed(logicIndex);
    }
}

void LocustTableWidgetItem::hideWidget()
{
    if (proxyItem) {
        disconnect(this, &LocustTableWidgetItem::itemShowed, tableWidget, &LocustTableWidget::itemShowed);

        tableWidget->scene->removeItem(proxyItem);
        proxyItem->setWidget(nullptr);
        delete  listItem;
        listItem = nullptr;
        delete proxyItem;
        proxyItem = nullptr;
    }
}

bool LocustTableWidgetItem::tryChangeExpand()
{
    if (listItem) {
        return listItem->changeExpand();
    }
    return false;
}

bool LocustTableWidgetItem::tryNotExpand()
{
    if (listItem && listItem->isExpand()) {
        listItem->changeExpand();
        return true;
    }
    return false;
}

int LocustTableWidgetItem::getHeight() const
{
    return height;
}


int LocustTableWidgetItem::getPos() const
{
    return leftTop;
}

void LocustTableWidgetItem::updateItem()
{
    if (proxyItem) {
        proxyItem->update();
    }
}

bool LocustTableWidgetItem::mousePress(const QPoint &pt, QMouseEvent *parentEvent)
{
    return listItem ? listItem->mousePress(pt, parentEvent) : false;
}

bool LocustTableWidgetItem::mouseRelease(const QPoint &pt, QMouseEvent *parentEvent)
{
    return listItem ? listItem->mouseRelease(pt, parentEvent) : false;
}

bool LocustTableWidgetItem::mouseDoubleClick(const QPoint &pt, QMouseEvent *parentEvent)
{
    return listItem ? listItem->mouseDoubleClick(pt, parentEvent) : false;
}

bool LocustTableWidgetItem::mouseMove(const QPoint &pt, QGraphicsSceneMouseEvent *parentEvent)
{
    return listItem ? listItem->mouseMove(pt, parentEvent) : false;
}

LocustMoreWidget::LocustMoreWidget()
    : QWidget()
    , isNoMore(false)
    , btnForceLoading(new QPushButton(this))
    , lbTip(new QLabel(this))
    , layout(new QHBoxLayout())
    , loadingWidget(new WaitingSpinnerWidget(this))
{
    lbTip->setText(tr("*********我也是有底线的*********"));
    lbTip->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
    btnForceLoading->setText(tr("重新加载"));

    layout->addStretch(1);
    layout->addWidget(lbTip);
    layout->addWidget(btnForceLoading);
    layout->addStretch(1);
    setLayout(layout);
}

LocustMoreWidget::~LocustMoreWidget()
{
    delete btnForceLoading;
    delete lbTip;
    delete layout;

    delete loadingWidget;
}

LocustFetchMoreItem::LocustFetchMoreItem(class LocustTableWidget *tableWidget)
    : tableWidget(tableWidget)
    , proxyItem(nullptr)
    , leftTop(0)
{
    proxyItem = new QGraphicsProxyWidget();
    tableWidget->scene->addItem(proxyItem);
    moreWidget = new LocustMoreWidget();
    moreWidget->setAcceptDrops(true);
    moreWidget->installEventFilter(tableWidget);
    proxyItem->setWidget(moreWidget);

    proxyItem->setGeometry(QRect(0, leftTop, tableWidget->headerview->width(), getHeight()));
    connect(tableWidget->headerview, &LocustTableWidgetHeaderView::sizeChanged, this, &LocustFetchMoreItem::headviewSizeChanged);
    connect(moreWidget->btnForceLoading, &QPushButton::clicked, this, &LocustFetchMoreItem::requireReload, Qt::QueuedConnection);
}


LocustFetchMoreItem::~LocustFetchMoreItem()
{
    tableWidget->scene->removeItem(proxyItem);
    proxyItem->setWidget(nullptr);
    delete  moreWidget;
    moreWidget = nullptr;
    delete proxyItem;
    proxyItem = nullptr;
}

void LocustFetchMoreItem::resetPos(int leftTop)
{
    this->leftTop = leftTop;
    proxyItem->setGeometry(QRect(0, leftTop, tableWidget->headerview->width(), getHeight()));
}

int LocustFetchMoreItem::getHeight() const
{
    return MORE_ITEM_HEIGHT;
}

void LocustFetchMoreItem::showLoading()
{
    moreWidget->isNoMore = false;

    moreWidget->loadingWidget->show();
    moreWidget->loadingWidget->start();
}

void LocustFetchMoreItem::hideLoading()
{
    moreWidget->loadingWidget->hide();
    moreWidget->loadingWidget->stop();
}

void LocustFetchMoreItem::changeToNoMore()
{
    moreWidget->isNoMore = true;
    moreWidget->btnForceLoading->setVisible(true);
}

void LocustFetchMoreItem::itemShowed()
{
    if (!moreWidget->isNoMore && !moreWidget->loadingWidget->isSpinning()) {
        emit requireLoading();
    }
}

void LocustFetchMoreItem::headviewSizeChanged(int width)
{
    if (proxyItem) {
        QRectF rt = proxyItem->geometry();
        proxyItem->setGeometry(QRectF(rt.topLeft(), QSizeF(width, getHeight())));
    }
}

LocustBaseTableView::LocustBaseTableView(QWidget *parent)
    : QTableView(parent)
{

}

void LocustBaseTableView::wheelEvent(QWheelEvent *event)
{
    event->ignore();
}

void LocustBaseTableView::leaveEvent(QEvent *event)
{
    QTableView::leaveEvent(event);

    emit mouseLeaved();
}

LocustTableWidgetBaseItemDelegate::LocustTableWidgetBaseItemDelegate(QObject *parent /*= nullptr*/)
    : QStyledItemDelegate(parent)
    , borderColor(QColor("#8FA9C2"))
{

}

LocustTableWidgetBaseItemDelegate::~LocustTableWidgetBaseItemDelegate()
{

}

void LocustTableWidgetBaseItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;


    painter->setPen(borderColor);
    QVariant bg = index.data(Qt::BackgroundRole);
    if (bg.isValid()) {
        painter->setBrush(bg.value<QBrush>());
    } else {
        if (index.row() & 1) {
            painter->setBrush(QColor("#F3F3F3"));
        } else {
            painter->setBrush(Qt::white);
        }
    }

    QRectF rt(opt.rect);
    painter->drawRect(rt.adjusted(-0.5, 0, -0.5, 0));

    if (index.column() == 0) {
        painter->drawLine(opt.rect.topLeft(), opt.rect.bottomLeft());
    }

    opt.text = index.data(Qt::DisplayRole).toString().replace('\n', ' ');
    if (!opt.text.isEmpty()) {
        QVariant value = index.data(Qt::FontRole);
        if (value.isValid() && !value.isNull()) {
            opt.font = qvariant_cast<QFont>(value).resolve(opt.font);
            opt.fontMetrics = QFontMetrics(opt.font);
        }
        painter->setFont(opt.font);

        QRect rt = opt.rect;
        int flag = Qt::TextSingleLine;

        if (index.column() == 0) {
            rt.adjust(20, 0, 0, 0);
            flag |= Qt::AlignVCenter | Qt::AlignLeft;
        } else {
            rt.adjust(1, 0, -1, 0);
            flag |= Qt::AlignCenter;
        }
        if (index.column() == 0) {
            painter->setPen(QColor("#F4EA84"));
        } else {
            painter->setPen(opt.palette.text().color());
        }

        const QString &elidedText = opt.fontMetrics.elidedText(opt.text, Qt::ElideRight, rt.width());
        painter->drawText(rt, flag, elidedText);
    }
}

LocustGraphicsView::LocustGraphicsView(QGraphicsScene *scene, QWidget *parent /*= nullptr*/)
    : QGraphicsView(scene, parent)
{
}


void LocustGraphicsView::setViewportMargins(int left, int top, int right, int bottom)
{
    QGraphicsView::setViewportMargins(left, top, right, bottom);
}

QMargins LocustGraphicsView::viewportMargins() const
{
    return QGraphicsView::viewportMargins();
}

void LocustGraphicsView::wheelEvent(QWheelEvent *event)
{
    // 屏蔽水平的
    if (qAbs(event->angleDelta().x()) > qAbs(event->angleDelta().y()))
        return;
    QGraphicsView::wheelEvent(event);
}

void LocustGraphicsView::keyPressEvent(QKeyEvent *event)
{
    QCoreApplication::sendEvent(scene(), event);
}

LocustGraphicsScene::LocustGraphicsScene(class LocustTableWidget *tableWidget, QObject *parent /*= nullptr*/)
    : QGraphicsScene(parent)
    , tableWidget(tableWidget)
{
}

void LocustGraphicsScene::helpEvent(QGraphicsSceneHelpEvent *event)
{
    //QGraphicsSceneHelpEvent 内部会设置tooltip,这里屏蔽了这个事件
    Q_UNUSED(event);
}

void LocustGraphicsScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (!tableWidget->sceneMouseMoveEvent(event)) {
        QGraphicsScene::mouseMoveEvent(event);
    }
}

void LocustGraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsScene::mouseReleaseEvent(event);
    tableWidget->sceneMouseReleaseEvent(event);
}

LocustTableWidget::LocustTableWidget(QWidget *parent)
    : QWidget(parent)
    , operations(new CoroutineGroup())
    , scene(new LocustGraphicsScene(this))
    , view(new LocustGraphicsView(scene, this))
    , mainModel(nullptr)
    , headerview(nullptr)
    , sortColumn(-1)
    , order(Qt::DescendingOrder)
    , sortRole(LocustTableWidget::ValueRole)
    , visualBegin(0)
    , visualEnd(-1)
    , tmUpdateShouldShow(-1)
    , tmResize(-1)
    , tmhBarDelayChange(-1)
    , lastContentHeight(0)
    , lastExpandVisualIndex(-1)
    , moreItem(nullptr)
    , hBar(new QScrollBar(Qt::Horizontal, this))
    , logicsSelectedItemsValid(true)
    , curSelectedItemVisualIndex(-1)
    , inMouseMove(false)
    , inPress(false)
    , curPressMouseMoveItemVisualIndex(-1)
{
    hBar->setVisible(false);
}

LocustTableWidget::~LocustTableWidget()
{
    delete operations;
    if (tmUpdateShouldShow != -1) {
        killTimer(tmUpdateShouldShow);
        tmUpdateShouldShow = -1;
    }
    if (tmResize != -1) {
        killTimer(tmResize);
        tmResize = -1;
    }
    if (tmhBarDelayChange != -1) {
        killTimer(tmhBarDelayChange);
        tmhBarDelayChange = -1;
    }
    for (LocustTableWidgetItem *item : listLogics) {
        delete item;
    }
    listLogics.clear();
    if (moreItem) {
        delete moreItem;
        moreItem = nullptr;
    }
    delete hBar;
    delete mainModel;
    delete headerview;
    delete view;
    delete scene;
}

#define  THREAD_COUNT 4
//排序过程中，原始数据的前count项需要保持不变
QVector<int> _sort(int count, int sortColumn, Qt::SortOrder order, int sortRole, QAbstractTableModel *shelfProductModel, std::function<bool(int)> filterAcceptsRow)
{
    QElapsedTimer timer;
    timer.start();

    QVector<int> listVisuals;
    QSharedPointer<QMap<int, QVariant>> cache;
    bool needSort = sortColumn >= 0 && sortColumn < shelfProductModel->columnCount(QModelIndex());
    if (needSort) {
        cache.reset(new QMap<int, QVariant>());
    }
    listVisuals.reserve(count);
    for (int i = 0; i < count; i++) {
        if (timer.elapsed() > 20) {
            timer.restart();
            Coroutine::msleep(0);
        }
        
        if (filterAcceptsRow(i)) {
            listVisuals.append(i);

            if (needSort) {
                const QVariant &value = shelfProductModel->index(i, sortColumn).data(sortRole);
                cache->insert(i, value);
            }
        }
    }

    if (needSort) {
        struct {
            bool isVariantLessThan(const QVariant &left, const QVariant &right,
                int A, int B) const
            {
                if (left.userType() == QMetaType::UnknownType)
                    return false;
                if (right.userType() == QMetaType::UnknownType)
                    return true;

#define RT_CMP_M_N return m == n ? A < B : m < n

                switch (left.userType()) {
                case QMetaType::Int:
                {
                    int m = left.toInt(), n = right.toInt();
                    RT_CMP_M_N;
                }
                case QMetaType::UInt:
                {
                    uint m = left.toUInt(), n = right.toUInt();
                    RT_CMP_M_N;
                }
                case QMetaType::LongLong:
                {
                    qlonglong m = left.toLongLong(), n = right.toLongLong();
                    RT_CMP_M_N;
                }
                case QMetaType::ULongLong:
                {
                    qulonglong m = left.toULongLong(), n = right.toULongLong();
                    RT_CMP_M_N;
                }
                case QMetaType::Float:
                {
                    float m = left.toFloat(), n = right.toFloat();
                    RT_CMP_M_N;
                }
                case QMetaType::Double:
                {
                    float m = left.toDouble(), n = right.toDouble();
                    RT_CMP_M_N;
                }
                case QMetaType::QChar:
                {
                    QChar m = left.toChar(), n = right.toChar();
                    RT_CMP_M_N;
                }
                case QMetaType::QDate:
                {
                    const QDate &m = left.toDate(), &n = right.toDate();
                    RT_CMP_M_N;
                }
                case QMetaType::QTime:
                {
                    const QTime &m = left.toTime(), &n = right.toTime();
                    RT_CMP_M_N;
                }
                case QMetaType::QDateTime:
                {
                    const QDateTime &m = left.toDateTime(), &n = right.toDateTime();
                    RT_CMP_M_N;
                }
                case QMetaType::QString:
                default:
                {
                    const QString &m = left.toString(), &n = right.toString();
                    int ret = m.compare(n);
                    return ret == 0 ? A < B : ret < 0;
                }
                }
            }
            bool operator()(int A, int B) const
            {
                const QVariant &left = cache->value(A);
                const QVariant &right = cache->value(B);

                if (order == Qt::DescendingOrder) {
                    return isVariantLessThan(right, left, B, A);
                }
                return isVariantLessThan(left, right, A, B);
            }
            Qt::SortOrder order;
            QSharedPointer<QMap<int, QVariant>> cache;
        } __cmp;

        __cmp.order = order;
        __cmp.cache = cache;

        if (listVisuals.size() < 10000) {
            std::sort(listVisuals.begin(), listVisuals.end(), __cmp);
        } else {
            struct CmpData {
                int begin;
                int end;
                QVector<int> *listVisuals;
                int cur;
            };

            int chunkSize = listVisuals.size() / THREAD_COUNT;

            QList<CmpData> listToCmp;
            for (int i = 0; i < THREAD_COUNT; i++) {
                CmpData data;
                data.begin = i * chunkSize;
                if (i == THREAD_COUNT - 1) {
                    data.end = listVisuals.size();
                } else {
                    data.end = (i + 1) * chunkSize;
                }
                data.cur = data.begin;
                data.listVisuals = &listVisuals;
                listToCmp.append(data);
            }

            ThreadPool pool(THREAD_COUNT);
            pool.each<CmpData>([__cmp] (const CmpData &data) {
                std::sort(data.listVisuals->begin() + data.begin, data.listVisuals->begin() + data.end, __cmp);
            }, listToCmp);

            //simple merge->o(4n)
            QVector<int> ret(count);
            for (int i = 0; i < listVisuals.size(); i++) {
                int minIndex;
                int minIndexThread = -1;
                for (int j = 0; j < THREAD_COUNT; j++) {
                    const int &cur = listToCmp[j].cur;
                    if (cur >= listToCmp[j].end) {
                        continue;
                    }
                    if (minIndexThread == -1) {
                        minIndex = cur;
                        minIndexThread = j;
                        continue;
                    }
                    if (__cmp(cur, minIndex)) {
                        minIndex = cur;
                        minIndexThread = j;
                    }
                }
                listToCmp[minIndexThread].cur++;
                ret[i] = listVisuals[minIndex];
            }
            return ret;
        }
    }
    return listVisuals;
}

void LocustTableWidget::init()
{
    mainModel = createMainModel();
    headerview = new LocustTableWidgetHeaderView(mainModel, this, view);
    //预留滚动条位置
    QSize rtHeader = headerview->sizeHint();
    view->setGeometry(0, 0, rtHeader.width() + SCROLLBAR_WIDTH, height() - SCROLLBAR_WIDTH);

    view->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    view->setRenderHint(QPainter::Antialiasing, false);
    view->setOptimizationFlags(QGraphicsView::DontSavePainterState);
    

    view->setObjectName("LocustGraphicsView");
    makeBeauty();

    QRect rt(QPoint(0, 0), rtHeader);
    lastContentHeight = rt.height();
    scene->setSceneRect(0, 0, rt.width() + FRAME_WIDTH * 2, rt.height());

    headerview->setSectionsClickable(true);
    headerview->getBaseHeaderView()->setSectionsClickable(true);
    headerview->setGeometry(FRAME_WIDTH, FRAME_WIDTH, rtHeader.width(), rtHeader.height());
    view->setViewportMargins(0, rt.height(), 0, 0);

    operations->spawnWithName("init", [this] {
        initSync(true);
    }, true);


    connect(headerview, &QHeaderView::sectionClicked, this, [this] (int section) {
        const QString &sectionName = mainModel->headerData(section, Qt::Horizontal, Qt::DisplayRole).toString();
        if (sectionName.isEmpty()) {
            return;
        }
        emit sectionClicked(section, sectionName);
    }, Qt::QueuedConnection);
    connect(headerview->getBaseHeaderView(), &QHeaderView::sectionClicked, this, [this] (int section) {
        LocustTableWidgetBaseHeaderView *baseHeaderView = headerview->getBaseHeaderView();
        QAbstractItemModel *model = baseHeaderView->model();
        if (!model) {
            return;
        }
        const QString &sectionName = model->headerData(section, Qt::Horizontal, Qt::DisplayRole).toString();
        emit baseHeaderSectionClicked(section, sectionName);
    }, Qt::QueuedConnection);

    connect(view->verticalScrollBar(), &QScrollBar::valueChanged, this, &LocustTableWidget::scrollPosChanged);
    connect(headerview, &QHeaderView::customContextMenuRequested, this, &LocustTableWidget::showHeaderViewCustomMenu);
    connect(headerview->getBaseHeaderView(), &QHeaderView::customContextMenuRequested, this, &LocustTableWidget::showHeaderViewCustomMenu);
    connect(headerview->getBaseHeaderView(), &QHeaderView::sectionResized, this, &LocustTableWidget::hBarRangeOrPageChange);
    connect(headerview, &QHeaderView::sectionResized, this, &LocustTableWidget::headerViewSectionResized);
    connect(headerview, &LocustTableWidgetHeaderView::sizeChanged, this, &LocustTableWidget::hBarRangeOrPageChange);
    connect(headerview, &LocustTableWidgetHeaderView::baseHeaderSizeChanged, this, &LocustTableWidget::hBarRangeOrPageChange);
}

int LocustTableWidget::getSortColumn() const
{
    return sortColumn;
}

Qt::SortOrder LocustTableWidget::getSortOrder() const
{
    return order;
}

int LocustTableWidget::getSortRole() const
{
    return sortRole;
}

void LocustTableWidget::sort(int sortIndex, Qt::SortOrder order, int sortRole)
{
    this->sortColumn = sortIndex;
    this->order = order;
    this->sortRole = sortRole;

    _sortedAsync();
}



void LocustTableWidget::tryChangeExpand()
{
    bool changed = false;
    for (int i = visualBegin; i <= visualEnd && i < listVisuals.size(); i++) {
        int logicsIndex = listVisuals[i];
        LocustTableWidgetItem *item = getItemWidget(logicsIndex);
        if (item) {
            changed |= item->tryNotExpand();
        }
    }
    if (changed) {
        return;
    }
    QPoint c = QCursor::pos();
    QPoint pt = view->mapFromGlobal(c);
    if (!view->rect().contains(pt)) {
        return;
    }
    int t = view->verticalScrollBar()->value() + pt.y() - headerview->height();
    int visualIndex = _findVisualIndexByPos(t);
    if (visualIndex >= visualBegin && visualIndex <= visualEnd) {
        int logicsIndex = listVisuals[visualIndex];
        LocustTableWidgetItem *item = getItemWidget(logicsIndex);
        if (item) {
            item->tryChangeExpand();
        }
    }
}

QVector<int> LocustTableWidget::visualArrays() const
{
    return listVisuals;
}

void LocustTableWidget::SetTips(const QString &tips)
{
    tipsWhenBaseTableUnvisible = tips;
}

QByteArray LocustTableWidget::saveSectionsSize() const
{
    QJsonObject obj(headerview->saveSectionsSize());
    obj.insert("sortColumn", sortColumn);
    obj.insert("sortOrder", (int)order);
    obj.insert("sortRole", sortRole);
    return QJsonDocument(obj).toJson();
}

void LocustTableWidget::restoreSectionSize(const QByteArray &data)
{
    const QJsonObject &obj = QJsonDocument::fromJson(data).object();
    headerview->restoreSectionSize(obj);
    sortColumn = obj.value("sortColumn").toInt(-1);
    order = (Qt::SortOrder)obj.value("sortOrder").toInt((int)Qt::DescendingOrder);
    sortRole = obj.value("sortRole").toInt(ValueRole);
}

void LocustTableWidget::reInitSectionsSize()
{
    headerview->reInitSectionsSize();
}

void LocustTableWidget::showFetchMoreItem(bool show)
{
    if (show) {
        if (!moreItem) {
            moreItem = new LocustFetchMoreItem(this);
            connect(moreItem, &LocustFetchMoreItem::requireLoading, this, &LocustTableWidget::requireLoadMore, Qt::QueuedConnection);
            connect(moreItem, &LocustFetchMoreItem::requireReload, this, &LocustTableWidget::requireReload, Qt::QueuedConnection);

            _resetPos(listVisuals);
        }
    } else {
        if (moreItem) {
            delete moreItem;
            moreItem = nullptr;
            _resetPos(listVisuals);
        }
    }
}

void LocustTableWidget::fetchMoreShowLoading()
{
    if (moreItem) {
        moreItem->showLoading();
    }
}

void LocustTableWidget::fetchMoreHideLoading()
{
    if (moreItem) {
        moreItem->hideLoading();
    }
}

bool LocustTableWidget::isItemSelected(int logicalIndex) const
{   //log(n)
    if (logicsSelectedItemsValid) {
        return logicsSelectedItems.find(logicalIndex) != logicsSelectedItems.end();
    }
    return selectedItemUserContents.find(mainModel->index(logicalIndex, 0).data(SrcIndexRole).toULongLong()) != selectedItemUserContents.end();
}

bool LocustTableWidget::selectItem(int logicalIndex, bool select /*= true*/)
{
    //qDebug() << "selectItem" << logicalIndex << select;
    if (logicalIndex < 0 || logicalIndex >= listLogics.size()) {
        return false;
    }
    if (!mainModel) {
        return false;
    }
    bool changed = false;
    if (logicsSelectedItemsValid) {
        if (select) {
            changed = logicsSelectedItems.insert(logicalIndex).second;
        } else {
            changed = !!logicsSelectedItems.erase(logicalIndex);
        }
    }
    quint64 userContent = mainModel->index(logicalIndex, 0).data(SrcIndexRole).toULongLong();
    if (select) {
        changed |= selectedItemUserContents.insert(userContent).second;
    } else {
        changed |= !!selectedItemUserContents.erase(userContent);
    }
    if (changed) {
        listLogics.at(logicalIndex)->updateItem();
    }
    return changed;
}

QList<int> LocustTableWidget::getSelectItemLogicIndexs() const
{
    if (!mainModel) {
        return QList<int>();
    }
    if (logicsSelectedItemsValid) {
        QList<int> ret;
        for (std::set<int>::const_iterator it = logicsSelectedItems.begin(); it != logicsSelectedItems.end(); it++) {
            ret.append(*it);
        }
        return ret;
    }
    //n*log(n)
    QList<int> ret;
    for (int i = visualBegin; i <= visualEnd && i < listVisuals.size(); i++) {
        int logicalIndex = listVisuals[i];
        quint64 userContent = mainModel->index(logicalIndex, 0).data(SrcIndexRole).toULongLong();
        if (selectedItemUserContents.find(userContent) != selectedItemUserContents.end()) {
            ret.append(logicalIndex);
        }
    }
    return ret;
}

QList<quint64> LocustTableWidget::getSelectItemUserContents() const
{
    if (!mainModel) {
        return QList<quint64>();
    }
    if (logicsSelectedItemsValid) {
        QList<quint64> ret;
        for (std::set<int>::const_iterator it = logicsSelectedItems.begin(); it != logicsSelectedItems.end(); it++) {
            quint64 userContent = mainModel->index(*it, 0).data(SrcIndexRole).toULongLong();
            ret.append(userContent);
        }
        return ret;
    }
    //n*log(n)
    QList<quint64> ret;
    for (int i = visualBegin; i <= visualEnd && i < listVisuals.size(); i++) {
        int logicalIndex = listVisuals[i];
        quint64 userContent = mainModel->index(logicalIndex, 0).data(SrcIndexRole).toULongLong();
        if (selectedItemUserContents.find(userContent) != selectedItemUserContents.end()) {
            ret.append(userContent);
        }
    }
    return ret;
}

void LocustTableWidget::clearSelects(bool updateView /*= false*/)
{
    //qDebug() << "clearSelects";
    if (!logicsSelectedItemsValid) {
        logicsSelectedItemsValid = true;
    }
    logicsSelectedItems.clear();
    selectedItemUserContents.clear();
    if (updateView) {
        for (int i = visualBegin; i <= visualEnd && i < listVisuals.size(); i++) {
            int logicsIndex = listVisuals[i];
            LocustTableWidgetItem *item = getItemWidget(logicsIndex);
            if (item) {
                item->updateItem();
            }
        }
    }
}

void LocustTableWidget::itemAboutToAdd(int logicsIndex)
{
    //if (logicsIndex < 0 || logicsIndex > listLogics.size()) {
    //    return;
    //}
}

void LocustTableWidget::itemAdded(int logicsIndex)
{
    if (logicsIndex < 0 || logicsIndex > listLogics.size()) {
        return;
    }

    LocustTableWidgetItem *item = new LocustTableWidgetItem(this);
    listLogics.append(item);

    _sortedAsync();
}

void LocustTableWidget::itemChanged(int logicsIndex)
{
    if (logicsIndex < 0 || logicsIndex >= listLogics.size()) {
        return;
    }
    LocustTableWidgetItem *item = listLogics.at(logicsIndex);
    item->resetLogicIndex(logicsIndex);
}

void LocustTableWidget::itemAboutToRemove(int logicsIndex)
{
    if (logicsIndex < 0 || logicsIndex >= listLogics.size()) {
        return;
    }

    hideAllItemSync();

    //对选择的数据进行操作
    if (logicsSelectedItemsValid && !logicsSelectedItems.empty()) {
        std::set<int>::const_reverse_iterator it = logicsSelectedItems.rbegin();
        int maxLogicItem = *(it);
        //后续其实可以优化
        if (logicsIndex <= maxLogicItem) {
            logicsSelectedItemsValid = false;
        }
    }
    quint64 userContent = mainModel ? mainModel->index(logicsIndex, 0).data(SrcIndexRole).toULongLong() : 0;
    selectedItemUserContents.erase(userContent);

    LocustTableWidgetItem *old = listLogics.takeAt(logicsIndex);
    delete old;

    int visualIndex = -1;
    for (int i = listVisuals.size() - 1; i >= 0; i--) {
        if (listVisuals[i] > logicsIndex) {
            --listVisuals[i];
        } else if (listVisuals[i] == logicsIndex) {
            visualIndex = i;
        }
    }

    int curSelectedItemLogics = curSelectedItemVisualIndex >= 0 && curSelectedItemVisualIndex < listVisuals.size() ? listVisuals.at(curSelectedItemVisualIndex) : -1;
    if (logicsIndex == curSelectedItemLogics) {
        curSelectedItemVisualIndex = -1;
    } else if (curSelectedItemLogics != -1 && visualIndex < curSelectedItemVisualIndex) {
        curSelectedItemVisualIndex--;
    }

    if (visualIndex != -1) {
        listVisuals.removeAt(visualIndex);
    }
    _resetPos(listVisuals);
}

void LocustTableWidget::itemRemoved(int logicsIndex)
{
    Q_UNUSED(logicsIndex);
}

void LocustTableWidget::itemAboutToReset()
{
    QList<LocustTableWidgetItem *> listLogics = this->listLogics;
    this->listLogics.clear();
    visualBegin = 0;
    visualEnd = -1;

    for (LocustTableWidgetItem *item : listLogics) {
        delete item;
    }

    clearSelects();
    curSelectedItemVisualIndex = -1;
}

void LocustTableWidget::itemReseted()
{
    operations->spawnWithName("init", [this] {
        initSync(false);
    }, true);
}

void LocustTableWidget::scrollPosChanged(int value)
{
    int pos = _findVisualIndexByPos(value);
    if (pos < visualBegin || pos > visualEnd) {
        _updateShouldShowList();
    } else {
        startTimerUpdateShouldShow();
    }
}

void LocustTableWidget::expandChanged(int visualIndex, int delta)
{
    if (delta > 0) {
        //收起其它所有的数据
        for (int i = visualBegin; i <= visualEnd && i < listVisuals.size(); i++) {
            if (i == visualIndex) {
                continue;
            }
            int logicsIndex = listVisuals[i];
            LocustTableWidgetItem *item = getItemWidget(logicsIndex);
            if (item) {
                item->tryNotExpand();
            }
        }
    }


    int contentHeight = 0;
    if (sumHeight.size() > 0) {
        contentHeight = sumHeight[sumHeight.size() - 1];
    }
    int expandTotalHeight = delta;
    if (visualIndex < listVisuals.size()) {
        expandTotalHeight += sumHeight[visualIndex];
    }
    if (contentHeight < expandTotalHeight) {
        contentHeight = expandTotalHeight;
    }
    if (moreItem) {
        contentHeight += moreItem->getHeight();
    }

    if (contentHeight != lastContentHeight) {
        lastContentHeight = contentHeight;
        scene->setSceneRect(0, 0, scene->width(), contentHeight);
    }
}

void LocustTableWidget::itemPressed(QMouseEvent *event, int logicsIndex, int visualIndex)
{
    if (event->button() == Qt::LeftButton) {
        if (event->modifiers() == Qt::ControlModifier) {
            selectItem(logicsIndex, !isItemSelected(logicsIndex));
        } else if (event->modifiers() == Qt::ShiftModifier) {
            if (curSelectedItemVisualIndex == -1) {
                return;
            }
            int begin = qMin(curSelectedItemVisualIndex, visualIndex);
            int end = qMax(curSelectedItemVisualIndex, visualIndex);
            clearSelects(!selectedItemUserContents.empty());
            for (int tmpVisualIndex = begin; tmpVisualIndex <= end && tmpVisualIndex < listVisuals.size(); tmpVisualIndex++) {
                selectItem(listVisuals.at(tmpVisualIndex));
            }
            return;
        } else {
            clearSelects(!selectedItemUserContents.empty());
            selectItem(logicsIndex);
        }
        curSelectedItemVisualIndex = visualIndex;

        //qDebug() << visualIndex << event->modifiers() << getSelectItemUserContents();
    } else if (event->button() == Qt::RightButton) {
        if (!isItemSelected(logicsIndex)) {
            clearSelects(!selectedItemUserContents.empty());
            selectItem(logicsIndex);
        }
    }
}

void LocustTableWidget::itemPressMouseMove(const QPoint &pt, bool ctrlPress)
{
    if (curSelectedItemVisualIndex == -1) {
        return;
    }

    int pos =  pt.y();
    int dstVisluaIndex = _findVisualIndexByPos(pos);
    if (dstVisluaIndex >= 0 && dstVisluaIndex < listVisuals.size()) {
        if (curPressMouseMoveItemVisualIndex == -1) {
            if (dstVisluaIndex == curSelectedItemVisualIndex) {
                //还是在原来那一项
                return;
            }
        } else if (dstVisluaIndex == curPressMouseMoveItemVisualIndex){
            return;
        }
    }

    if (inMouseMove) {
        return;
    }
    inMouseMove = true;
    curPressMouseMoveItemVisualIndex = dstVisluaIndex;

    QScrollBar *bar = view->verticalScrollBar();
    if (pos < bar->value()) {
        //需要向上翻滚
        makeSureVisible(dstVisluaIndex);
    } else {
        int height = view->rect().height() - view->viewportMargins().top();
        int bottom = bar->value() + height;
        if (pos > bottom) {
            //需要向下翻滚
            makeSureVisible(dstVisluaIndex);
        }
    }

    //先暴力，待后续优化
    int begin = qMin(curSelectedItemVisualIndex, dstVisluaIndex);
    int end = qMax(curSelectedItemVisualIndex, dstVisluaIndex);
    if (!ctrlPress) {
        clearSelects(!selectedItemUserContents.empty());
    }
    //todo:按ctrl时范围取消选取
    for (int tmpVisualIndex = begin; tmpVisualIndex <= end && tmpVisualIndex < listVisuals.size(); tmpVisualIndex++) {
        selectItem(listVisuals.at(tmpVisualIndex));
    }
    inMouseMove = false;
}

void LocustTableWidget::makeSureVisible(int visualIndex)
{
    if (visualIndex >= sumHeight.size()) {
        return;
    }
    QScrollBar *bar = view->verticalScrollBar();
    int top = bar->value();
    int height = view->rect().height() - view->viewportMargins().top();
    int bottom = top + height;
    int scrollPos = sumHeight[visualIndex];
    if (scrollPos > bottom) {
        bar->setValue(scrollPos - height);
    } else {
        scrollPos = visualIndex > 0 ? sumHeight[visualIndex - 1] : 0;
        if (scrollPos < top) {
            bar->setValue(scrollPos);
        }
    }
}

void LocustTableWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    bool initGeometry = property("initGeometry").toBool();
    if (!initGeometry) {
        setProperty("initGeometry", true);
        _resizeEvent();
        return;
    }
    if (tmResize == -1) {
        tmResize = startTimer(100);
    }
}

void LocustTableWidget::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == tmUpdateShouldShow) {
        _updateShouldShowList();
    } else if (event->timerId() == tmResize) {
        _resizeEvent();
    } else if (event->timerId() == tmhBarDelayChange) {
        _hBarRangeOrPageChange();
    }
}

bool LocustTableWidget::eventFilter(QObject *obj, QEvent *e)
{
    switch (e->type()) {
    case QEvent::DragEnter:
    {
        QDragEnterEvent *dragEvent = static_cast<QDragEnterEvent *>(e);
        QApplication::instance()->notify(this, e);
        if (e->isAccepted()) {
            dragEvent->setDropAction(Qt::CopyAction);
            return true;
        }
        return false;
    }
    case QEvent::DragMove:
    {
        QWidget *w = static_cast<QWidget *>(obj);
        if (w && w->acceptDrops()) {
            e->accept();
            return true;
        }
        break;
    }
    default:
        break;
    }
    return QWidget::eventFilter(obj, e);
}

void LocustTableWidget::wheelEvent(QWheelEvent *e)
{
    if (qAbs(e->angleDelta().x()) > qAbs(e->angleDelta().y()))
        QCoreApplication::sendEvent(hBar, e);

}

void LocustTableWidget::keyPressEvent(QKeyEvent *event)
{
    if (!mainModel) {
        return;
    }
    //为了往父控件传消息
    event->ignore();

    int dstCurSelectedItemVisualIndex = curSelectedItemVisualIndex;
    switch (event->key()) {
    case Qt::Key_Up:
    case Qt::Key_W:
        if (!listVisuals.isEmpty()) {
            if (dstCurSelectedItemVisualIndex == -1) {
                dstCurSelectedItemVisualIndex = 0;
            }
            dstCurSelectedItemVisualIndex = (dstCurSelectedItemVisualIndex - 1 + listVisuals.size()) % (listVisuals.size());
        }
        break;
    case Qt::Key_S:
    case Qt::Key_Down:
        if (!listVisuals.isEmpty()) {
            dstCurSelectedItemVisualIndex = (dstCurSelectedItemVisualIndex + 1) % (listVisuals.size());
        }
        break;
    }

    if (dstCurSelectedItemVisualIndex != curSelectedItemVisualIndex) {
        event->accept();
        clearSelects(!selectedItemUserContents.empty());
        int logicIndex = listVisuals.at(dstCurSelectedItemVisualIndex);
        selectItem(logicIndex);

        curSelectedItemVisualIndex = dstCurSelectedItemVisualIndex;
        makeSureVisible(curSelectedItemVisualIndex);
        return;
    }
    if (event == QKeySequence::SelectAll) {
        clearSelects(false);
        for (int i = 0; i < listVisuals.size(); i++) {
            selectItem(listVisuals[i]);
        }
        event->accept();
    } else if (event == QKeySequence::Copy) {
        emit doCopy();
        event->accept();
    }
}

void LocustTableWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    QScrollBar *bar = view->verticalScrollBar();
    int y = event->pos().y() + bar->value() - view->viewportMargins().top();
    int visualIndex = _findVisualIndexByPos(y, false);
    if (visualIndex < 0 || visualIndex >= listVisuals.size()) {
        return;
    }
    int logicsIndex = listVisuals.at(visualIndex);
    LocustTableWidgetItem *item = getItemWidget(logicsIndex);
    if (!item) {
        return;
    }
    QPoint pos(event->pos().x()/* - view->viewportMargins().left()*/, visualIndex == 0 ? y : y - sumHeight[visualIndex - 1]);
    if (item->mouseDoubleClick(pos, event)) {
        return;
    }
}

void LocustTableWidget::mousePressEvent(QMouseEvent *event)
{
    QScrollBar *bar = view->verticalScrollBar();
    int y = event->pos().y() + bar->value() - view->viewportMargins().top();
    //qDebug() << "mouse press" << y;
    int visualIndex = _findVisualIndexByPos(y, false);
    if (visualIndex < 0 || visualIndex >= listVisuals.size()) {
        return;
    }
    int logicsIndex = listVisuals.at(visualIndex);
    LocustTableWidgetItem *item = getItemWidget(logicsIndex);
    if (!item) {
        return;
    }
    QPoint pos(event->pos().x()/* - view->viewportMargins().left()*/, visualIndex == 0 ? y : y - sumHeight[visualIndex - 1]);
    if (item->mousePress(pos, event)) {
        return;
    }
    inPress = true;
    itemPressed(event, logicsIndex, visualIndex);
    event->accept();
}

void LocustTableWidget::mouseReleaseEvent(QMouseEvent *event)
{
    QWidget::mousePressEvent(event);
    inPress = false;
}

bool LocustTableWidget::sceneMouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    //qDebug() << "tableWidget mouse move(1)" << event->pos() << event->screenPos() << event->scenePos() << event->button() << event->buttons();
    if (event->buttons() & Qt::LeftButton && inPress) {
        itemPressMouseMove(event->scenePos().toPoint(), event->modifiers() == Qt::ControlModifier);
        return true;
    }

    int y = event->scenePos().y();
    
    int visualIndex = _findVisualIndexByPos(y, false);
    if (visualIndex < 0 || visualIndex >= listVisuals.size()) {
        return false;
    }
    int logicsIndex = listVisuals.at(visualIndex);
    LocustTableWidgetItem *item = getItemWidget(logicsIndex);
    if (!item) {
        return false;
    }
    QPoint pos(event->scenePos().x()/* - view->viewportMargins().left()*/, visualIndex == 0 ? y : y - sumHeight[visualIndex - 1]);
    if (item->mouseMove(pos, event)) {
        return true;
    }
    return false;
}

void LocustTableWidget::sceneMouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    //qDebug() << "sceneMouseReleaseEvent";
    inPress = false;
    QScrollBar *bar = view->verticalScrollBar();

    int y = event->scenePos().y();
    y += bar->value() - view->viewportMargins().top();

    int visualIndex = _findVisualIndexByPos(y, false);
    if (visualIndex < 0 || visualIndex >= listVisuals.size()) {
        return;
    }
    int logicsIndex = listVisuals.at(visualIndex);
    LocustTableWidgetItem *item = getItemWidget(logicsIndex);
    if (!item) {
        return;
    }
    QPoint pos(event->pos().x()/* - view->viewportMargins().left()*/, visualIndex == 0 ? y : y - sumHeight[visualIndex - 1]);
    QMouseEvent e(QEvent::KeyRelease, pos, event->screenPos(), event->button(),
        event->buttons(), event->modifiers());
    
    if (item->mouseRelease(pos, &e)) {
        return;
    }
}

bool LocustTableWidget::filterAcceptsRow(int logicIndex) const
{
    return true;
}

int LocustTableWidget::baseTableFirstSectionDefaultSize()
{
    return TABLEPLAN_FIRST_COLOUMN_WIDTH;
}

int LocustTableWidget::firstColumnDefaultWidth()
{
    return TABLE_FIRST_COLUMN_WIDTH;
}

int LocustTableWidget::itemToShowCount()
{
    if (mainModel) {
        return mainModel->rowCount();
    }
    return 0;
}

QAbstractItemDelegate *LocustTableWidget::createBaseItemDelegate()
{
    return new LocustTableWidgetBaseItemDelegate();
}

int LocustTableWidget::defaultItemHeight()
{
    return ITEM_HEIGHT_NOEXPAND;
}

int LocustTableWidget::defaultBaseRowWantShow()
{
    return TABLEPLAN_ROW_WHAT_SHOW_DEFAULT;
}

int LocustTableWidget::iconSize()
{
    return IMAGE_SIZE;
}

int LocustTableWidget::fontPointSize()
{
    return 8;
}

int LocustTableWidget::baseTableDefaultRowHeight()
{
    return TABLEPLAN_ROW_HEIGHT;
}

int LocustTableWidget::mainHeaderViewHeight() const
{
    return HEADERVIEW_HEIGHT >> 1;
}

bool LocustTableWidget::baseTableFirstSectionTextLeftAlign() const
{
    return true;
}

bool LocustTableWidget::enableBaseTableHoverItem() const
{
    return true;
}

void LocustTableWidget::makeBeauty()
{
    view->setStyleSheet(QString("QAbstractScrollArea#LocustGraphicsView{border:1px solid %1}").arg(QColor("#8FA9C2").name()));
}

void LocustTableWidget::initSync(bool needShowLoading)
{
    if (needShowLoading) {
        emit showLoading();
    }
    QElapsedTimer timer;
    timer.start();
    QList<LocustTableWidgetItem *> listLogics;
    int count = itemToShowCount();
    for (int i = 0; i < count; i++) {
        if (timer.elapsed() > 20) {
            timer.restart();
            Coroutine::msleep(0);
        }
        LocustTableWidgetItem *item = new LocustTableWidgetItem(this);
        listLogics.push_back(item);

        connect(item, &LocustTableWidgetItem::expandChanged, this, &LocustTableWidget::expandChanged);
    }
    this->listLogics = listLogics;

    _sortedAsync(needShowLoading);
}

void LocustTableWidget::_resetPos(const QVector<int> &listVisuals, bool showImmediately /*= false*/)
{
    hideAllItemSync();
    this->listVisuals = listVisuals;

    int pos = -1;
    int sum = 0;

    sumHeight.clear();
    sumHeight.resize(this->listVisuals.size());
    for (int i = 0; i < this->listVisuals.size(); i++) {
        LocustTableWidgetItem *item = listLogics.at(this->listVisuals[i]);
        item->resetPos(pos);

        int h = item->getHeight() - 1;

        sum += h;
        sumHeight[i] = sum;

        pos += h;
    }

    int contentHeight = 0;
    if (sumHeight.size() > 0) {
        contentHeight = sumHeight[sumHeight.size() - 1];
    }
    if (moreItem) {
        moreItem->resetPos(pos);
        contentHeight += moreItem->getHeight();
    }

    if (contentHeight != lastContentHeight) {
        lastContentHeight = contentHeight;
        scene->setSceneRect(0, 0, scene->width(), contentHeight);
    }


    if (showImmediately) {
        _updateShouldShowList();
    } else {
        startTimerUpdateShouldShow();
    }
}

void LocustTableWidget::_sortedAsync(bool needShowLoading /*= false*/)
{
    if (needShowLoading) {
        emit showLoading();
    }
    operations->spawnWithName("sort", [this, needShowLoading] {
        _resetPos(_sort(this->listLogics.size(), sortColumn, order, sortRole, mainModel, std::bind(&LocustTableWidget::filterAcceptsRow, this, std::placeholders::_1)), true);
        if (needShowLoading) {
            emit hideLoading();
        }
    }, true);
}

void LocustTableWidget::_resizeEvent()
{
    if (tmResize != -1) {
        killTimer(tmResize);
        tmResize = -1;
    }

    view->setGeometry(0, 0, width(), height() - SCROLLBAR_WIDTH - FRAME_WIDTH);
    if (headerview) {
        headerview->setGeometry(FRAME_WIDTH, FRAME_WIDTH, width() - FRAME_WIDTH - SCROLLBAR_WIDTH, headerview->height());
        hBar->setGeometry(FRAME_WIDTH + headerview->sectionSize(0), height() - SCROLLBAR_WIDTH - FRAME_WIDTH, width() - SCROLLBAR_WIDTH - FRAME_WIDTH * 2 - headerview->sectionSize(0), SCROLLBAR_WIDTH);
    }
    _updateShouldShowList();
}

void LocustTableWidget::_updateShouldShowList()
{
    if (tmUpdateShouldShow != -1) {
        killTimer(tmUpdateShouldShow);
        tmUpdateShouldShow = -1;
    }

    int top = view->verticalScrollBar()->value();
    int height = view->rect().height() - view->viewportMargins().top();
    int bottom = top + height + 0.15 * height;

    top -= 0.15 * height;

    int lastBegin = qMax(visualBegin, 0);
    int lastEnd = qMin(visualEnd, listVisuals.size() - 1);

    visualBegin = _findVisualIndexByPos(top);
    visualEnd = _findVisualIndexByPos(bottom);

    visualBegin = qMax(visualBegin - 1, 0);
    visualEnd = qMin(visualEnd + 1, listVisuals.size() - 1);

    for (int i = lastBegin; i <= lastEnd; i++) {
        if (i >= visualBegin && i <= visualEnd) {
            continue;
        }
        int logicsIndex = listVisuals[i];
        LocustTableWidgetItem *item = getItemWidget(logicsIndex);
        if (item) {
            item->hideWidget();
        }
    }

    for (int i = visualBegin; i <= visualEnd; i++) {
        int logicsIndex = listVisuals[i];
        LocustTableWidgetItem *item = getItemWidget(logicsIndex);
        if (item) {
            item->showWidget(logicsIndex, i);
        }
    }
    if (moreItem && visualEnd == listVisuals.size() - 1) {
        moreItem->itemShowed();
    }
}

void LocustTableWidget::hideAllItemSync()
{
    for (int i = visualBegin; i <= visualEnd && i < listVisuals.size(); i++) {
        int logicsIndex = listVisuals[i];
        LocustTableWidgetItem *item = getItemWidget(logicsIndex);
        if (item) {
            item->hideWidget();
        }
    }
    visualBegin = 0;
    visualEnd = -1;
}

int LocustTableWidget::_findVisualIndexByPos(int pos, bool nearest /*= false*/)
{
    if (pos < 0) {
        if (nearest) {
            return 0;
        }
        return -1;
    }
    
    int l = 0, r = sumHeight.size() - 1, mid;
    int ret = nearest ? r : r + 1;

    while (l <= r) {
        mid = (l + r) >> 1;
        if (sumHeight[mid] >= pos) {
            ret = qMin(mid, ret);
            r = mid - 1;
        } else {
            l = mid + 1;
        }
    }
    return ret;
}

LocustTableWidgetItem *LocustTableWidget::getItemWidgetByPos(int pos)
{
    int dstVisluaIndex = _findVisualIndexByPos(pos, false);
    if (dstVisluaIndex < 0 || dstVisluaIndex >= listVisuals.size()) {
        return nullptr;
    }
    return getItemWidget(listVisuals.at(dstVisluaIndex));
}

void LocustTableWidget::startTimerUpdateShouldShow()
{
    if (tmUpdateShouldShow == -1) {
        tmUpdateShouldShow = startTimer(10);
    }
}

LocustTableWidgetItem *LocustTableWidget::getItemWidget(int logicsIndex)
{
    if (logicsIndex < 0 || logicsIndex >= listLogics.size()) {
        return nullptr;
    }
    return listLogics.at(logicsIndex);
}

LocustTableWidgetHeaderView *LocustTableWidget::getHeaderview() const
{
    return headerview;
}

LocustTableWidgetBaseHeaderView *LocustTableWidget::getBaseHeaderview() const
{
    return headerview->getBaseHeaderView();
}

void LocustTableWidget::_hBarRangeOrPageChange()
{
    if (tmhBarDelayChange != -1) {
        killTimer(tmhBarDelayChange);
        tmhBarDelayChange = -1;
    }

    int w = headerview->getBaseHeaderWidth();
    int baseWidthHint = headerview->getBaseHeaderWidthHint();
    
    if (baseWidthHint > w) {
        if (!hBar->isVisible()) {
            if (QGuiApplication::mouseButtons() & Qt::LeftButton) {
                tmhBarDelayChange = startTimer(10);
            } else {
                hBar->setVisible(true);
            }
        }

        hBar->setPageStep(w);
        hBar->setSingleStep(w / 20);
        hBar->setMaximum(baseWidthHint - w);
    } else {
        if (hBar->isVisible()) {
            if (QGuiApplication::mouseButtons() & Qt::LeftButton) {
                tmhBarDelayChange = startTimer(10);
            } else {
                hBar->setVisible(false);
            }
        }

        hBar->setMaximum(0);
        hBar->setValue(0);
    }
}

void LocustTableWidget::hBarRangeOrPageChange()
{
    if (tmhBarDelayChange == -1) {
        tmhBarDelayChange = startTimer(10);
    }
}

void LocustTableWidget::headerViewSectionResized(int logicalIndex, int oldSize, int newSize)
{
    if (logicalIndex == 0) {
        hBar->setGeometry(FRAME_WIDTH + headerview->sectionSize(0), height() - SCROLLBAR_WIDTH - FRAME_WIDTH, width() - SCROLLBAR_WIDTH - FRAME_WIDTH * 2 - headerview->sectionSize(0), SCROLLBAR_WIDTH);
    }
}
