#ifndef LOCUSTTABLEWIDGET_H
#define LOCUSTTABLEWIDGET_H

#include <QTableView>
#include <QPushButton>
#include <QHeaderView>
#include <QLabel>
#include <QGraphicsView>
#include <QGraphicsProxyWidget>
#include <QStyledItemDelegate>
#include <set>

#include "qtnetworkng.h"

using namespace qtng;

class LocustTableWidgetBaseHeaderView : public QHeaderView
{
    Q_OBJECT
public:
    LocustTableWidgetBaseHeaderView(QWidget *parent);
protected:
    virtual QJsonObject saveBaseSectionsSize() const;
    virtual void restoreBaseSectionSize(const QJsonObject &obj);
    void _paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const;
    virtual void paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const override;
    virtual void showEvent(QShowEvent *) override;
    virtual void hideEvent(QHideEvent *) override;
signals:
    void showedChanged();
public:
    friend class LocustTableWidgetHeaderView;
    QVariant firstColumnColor;
    QColor bgColor;
    QColor firstColmnBgColor;
    QColor borderColor;
    bool textLeftFirstSection;  //第一列是否要左对齐
};


class LocustTableWidgetHeaderView : public LocustTableWidgetBaseHeaderView
{
    Q_OBJECT
public:
    LocustTableWidgetHeaderView(QAbstractItemModel *model, class LocustTableWidget *tableWidget, QWidget *parent);
    ~LocustTableWidgetHeaderView();
public:
    virtual QSize sizeHint() const override;
    int getBaseHeaderWidthHint() const;
    int getBaseHeaderWidth() const;
    void switchBaseTable();
    LocustTableWidgetBaseHeaderView *getBaseHeaderView() const;
    QJsonObject saveSectionsSize() const;
    void restoreSectionSize(const QJsonObject &obj);
    void reInitSectionsSize();
    void setBaseHeaderViewBgColor(const QColor &c);
signals:
    void sizeChanged(int width);
    void baseHeaderSizeChanged();
protected:
    void _sectionResized(int logicalIndex, int oldSize, int newSize);
    void _baseSectionResized(int logicalIndex, int oldSize, int newSize);
    void updateSubHeaderViewGeometry();
    void makeBeauty();
protected:
    virtual void paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const override;
    virtual void resizeEvent(QResizeEvent *event) override;
    virtual void paintEvent(QPaintEvent *event) override;
protected:
    class LocustTableWidget *tableWidget;
    LocustTableWidgetBaseHeaderView *baseHeaderView;
    QAbstractTableModel *baseTableModel;
    QWidget *userWidget;
    QString curBaseTableName;
    int widthHint;
    int baseWidthHint;
};


class LocustBaseTableView : public QTableView
{
    Q_OBJECT
public:
    LocustBaseTableView(QWidget *parent = nullptr);
signals:
    void mouseLeaved();
protected:
    void wheelEvent(QWheelEvent *event) override;
    void leaveEvent(QEvent *event) override;
};


class LocustTableWidgetBaseItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    LocustTableWidgetBaseItemDelegate(QObject *parent = nullptr);
    ~LocustTableWidgetBaseItemDelegate();
public:
    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
protected:
    QColor borderColor;
};



class LocustTableWidgetItemView : public QWidget
{
    Q_OBJECT
public:
    LocustTableWidgetItemView(class LocustTableWidget *tableWidget, int logicIndex, int visualIndex, bool expand, QWidget *parent = nullptr);
    ~LocustTableWidgetItemView();

    void dataChanged(int logicIndex, int heightBefore);
    bool changeExpand();
    bool isExpand();

    virtual bool mousePress(const QPoint &pt, QMouseEvent *parentEvent);
    virtual bool mouseRelease(const QPoint &pt, QMouseEvent *parentEvent);
    virtual bool mouseDoubleClick(const QPoint &pt, QMouseEvent *parentEvent);
    virtual bool mouseMove(const QPoint &pt, QGraphicsSceneMouseEvent *parentEvent);
signals:
    void heightChanged(int visualIndex, int height);
    void openBaseTable(int logicsIndex, int logicsBaseTable);
    void openMainItem(int logicIndex);
    void showBaseTableCustomMenu(int logicsIndex, int logicsBaseTable);
    void showCustomMenu(int logicsIndex);
    void baseTableHovered(int logicsIndex, int logicsBaseTable, const QRect &gRect);
    void iconHovered(int logicsIndex, const QRect &iconRt);
    void acceptBaseTable(int logicsIndex, int logicsBaseTable, bool accpet);
    void openLink(const QUrl &link);
    void wantChangeExpand(bool expand);
protected:
    virtual void resizeEvent(QResizeEvent *event) override;
    virtual void paintEvent(QPaintEvent *event) override;
    virtual void leaveEvent(QEvent *event) override;
    virtual void enterEvent(QEvent *event) override;
protected:
    int getSectionSize(int visualIndex);
    void updateTableGeometry();
    void updateUserWidgetGeometry();
    void updateUserDefineWidgetGeometry();
    void updateBtnGeometry();
    void makeBeauty();
    void acceptBtnClicked();
    void refuseBtnClicked();

    void headerViewSectionResized(int logicalIndex, int oldSize, int newSize);
    void subHeaderViewSectionResized(int logicalIndex, int oldSize, int newSize);
    void expandItem(bool expand);
    void modifyBaseTable(const QModelIndex &index);
    void openBaseTableCustomMenu(const QPoint &pos);
    void openCustomMenu(const QPoint &pos);
    void baseTableEntered(const QModelIndex &index);
    void initExpand();
    void resetExpand(int heightBefore);
    void initBtns();
    QString makeDetailHtml(const QString &detail, const QString &dataAddress, const QString &detail2, const QColor &fontColor);
    void paintDetail(const QString &detail, const QString &dataAddress, const QString &detail2, const QRect &rtDetail, QPainter &painter, const QColor &fontColor, class QTextDocument *doc);
protected:
    //小表相关
    void initBaseTable();
    void resetBaseTable();

    void expandChanged();
    
protected:
    int logicIndex;
    int visualIndex;
    class LocustTableWidget *tableWidget;
    bool expand;

    LocustBaseTableView *baseTable;
    QAbstractTableModel *baseTableModel;
    QAbstractItemDelegate *baseTableItemDelegate;
    QLabel *lbHover;
    QLabel *lbNum;

    QPushButton *btnAccept;
    QPushButton *btnRefuse;

    QPushButton *btnExpand;
    class QTextDocument *docLeft;
    class QTextDocument *docRight;

    QRect docRtLeft;
    QRect docRtRight;
    bool dataAddressLeft;

    QWidget *userWidget;//供用户自定义
    QList<QWidget *> userDefineWidgets;
};



class LocustTableWidgetItem : public QObject
{
    Q_OBJECT
public:
    LocustTableWidgetItem(class LocustTableWidget *tableWidget);
    ~LocustTableWidgetItem();
signals:
    void expandChanged(int visualIndex, int delta);
    void itemShowed(int logicIndex);
protected:
    void heightChanged(int visualIndex, int height);
    void headviewSizeChanged(int width);
public:
    void resetPos(int leftTop);
    void resetLogicIndex(int logicIndex);
    void showWidget(int logicIndex, int visualIndex);
    void hideWidget();
    bool tryChangeExpand();
    bool tryNotExpand();
public:
    int getHeight() const;
    int getPos() const;
    void updateItem();

    bool mousePress(const QPoint &pt, QMouseEvent *parentEvent);
    bool mouseRelease(const QPoint &pt, QMouseEvent *parentEvent);
    bool mouseDoubleClick(const QPoint &pt, QMouseEvent *parentEvent);
    bool mouseMove(const QPoint &pt, QGraphicsSceneMouseEvent *parentEvent);
protected:
    class LocustTableWidget *tableWidget;

    LocustTableWidgetItemView *listItem;
    QGraphicsProxyWidget *proxyItem;

    int height;         //实际高度
    int leftTop;        //其实这个变量可以优化掉(先不优化了)
};


class LocustMoreWidget : public QWidget
{
public:
    LocustMoreWidget();
    ~LocustMoreWidget();
protected:
    friend class LocustFetchMoreItem;
    bool isNoMore;  //没有更多可以加载了

    QPushButton *btnForceLoading;
    QLabel *lbTip;
    class QHBoxLayout *layout;

    class WaitingSpinnerWidget *loadingWidget;
};


class LocustFetchMoreItem : public QObject
{
    Q_OBJECT
public:
    LocustFetchMoreItem(class LocustTableWidget *tableWidget);
    ~LocustFetchMoreItem();
signals:
    void requireLoading();
    void requireReload();
public:
    void resetPos(int leftTop);
    int getHeight() const;

    void showLoading();
    void hideLoading();
    void changeToNoMore();

    void itemShowed();
protected:
    void headviewSizeChanged(int width);
    class LocustTableWidget *tableWidget;

    QGraphicsProxyWidget *proxyItem;
    LocustMoreWidget *moreWidget;

    int leftTop;
};


class LocustGraphicsView : public QGraphicsView
{
public:
    LocustGraphicsView(QGraphicsScene *scene, QWidget *parent = nullptr);
public:
    void setViewportMargins(int left, int top, int right, int bottom);
    QMargins viewportMargins() const;
protected:
    virtual void wheelEvent(QWheelEvent *event) override;
    virtual void keyPressEvent(QKeyEvent *event) override;
};

class LocustGraphicsScene : public QGraphicsScene
{
public:
    explicit LocustGraphicsScene(class LocustTableWidget *tableWidget, QObject *parent = nullptr);
protected:
    virtual void helpEvent(QGraphicsSceneHelpEvent *event) override;
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
private:
    class LocustTableWidget *tableWidget;
};


class LocustTableWidget : public QWidget
{
    Q_OBJECT
public:
    enum LocustUserRole{
        DetailRole = Qt::UserRole + 1,
        DetailSecondRole,
        DetailSecondBottomRole,
        FaceRole,
        SpecialNoteRole,
        NotesRole,
        DataAddressRole,
        SrcIndexRole,
        ShowAcceptRefuseBtnRole,
        ExpandRole,
        ValueRole,
        UserDefineWidgetColumnListRole, // 获取哪一些列是自定义控件(要求从小到大返回)
        UserDefineWidgetRole,           // 获取用户自定义控件样式
    };
    explicit LocustTableWidget(QWidget *parent);
    ~LocustTableWidget();
signals:
    void openMainItem(int logicsIndex);
    void openBaseTable(int logicsIndex, int logicsBaseTable);
    void showBaseTableCustomMenu(int logicsIndex, int logicsBaseTable);
    void showCustomMenu(int logicsIndex);
    void showHeaderViewCustomMenu(const QPoint &pos);
    void itemShowed(int logicIndex);
    void showLoading();
    void hideLoading();
    void sectionClicked(int logicalIndex, const QString &sectionName);
    void baseHeaderSectionClicked(int logicalIndex, const QString &sectionName);
    void baseTableHovered(int logicsIndex, int logicsBaseTable, const QRect &gRect);
    void mainItemIconHovered(int logicsIndex, const QRect &gRect);
    void acceptBaseTable(int logicsIndex, int logicsBaseTable, bool accpet);
    void openLink(const QUrl &link);
    void requireLoadMore();
    void requireReload();
    void doCopy();
public:
    virtual void init(); //必须调用并初始化filter ->后续看怎么优化掉

    int getSortColumn() const;
    Qt::SortOrder getSortOrder() const;
    int getSortRole() const;
    void sort(int sortIndex, Qt::SortOrder order, int sortRole);

    void tryChangeExpand();

    QVector<int> visualArrays() const;
    void SetTips(const QString &tips);

    QByteArray saveSectionsSize() const;
    void restoreSectionSize(const QByteArray &data);
    void reInitSectionsSize();

    void showFetchMoreItem(bool show);
    void fetchMoreShowLoading();
    void fetchMoreHideLoading();

    //需要实现index.data(SrcIndexRole)
    bool isItemSelected(int logicalIndex) const;
    bool selectItem(int logicalIndex, bool select = true);
    QList<int> getSelectItemLogicIndexs() const;
    QList<quint64> getSelectItemUserContents() const;
    void clearSelects(bool updateView = false);
protected:
    void itemAboutToAdd(int logicsIndex);
    void itemAdded(int logicsIndex);
    void itemChanged(int logicsIndex);
    void itemAboutToRemove(int logicsIndex);
    void itemRemoved(int logicsIndex);
    void itemAboutToReset();
    void itemReseted();

    void scrollPosChanged(int value);
    void expandChanged(int visualIndex, int delta);

    void itemPressed(QMouseEvent *event, int logicsIndex, int visualIndex);
    void itemPressMouseMove(const QPoint &pt, bool ctrlPress);
    void makeSureVisible(int visualInex);
protected:
    virtual void resizeEvent(QResizeEvent *event) override;
    virtual void timerEvent(QTimerEvent *event) override;
    virtual bool eventFilter(QObject *obj, QEvent *event) override;
    virtual void wheelEvent(QWheelEvent *event) override;
    virtual void keyPressEvent(QKeyEvent *event) override;

    virtual void mouseDoubleClickEvent(QMouseEvent *event) override;
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseReleaseEvent(QMouseEvent *event) override;
    bool sceneMouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void sceneMouseReleaseEvent(QGraphicsSceneMouseEvent *event);

    virtual bool filterAcceptsRow(int logicIndex) const;
    virtual int baseTableFirstSectionDefaultSize();
    virtual int firstColumnDefaultWidth();
    virtual QAbstractTableModel *createMainModel() = 0;
    virtual int itemToShowCount();
    virtual QAbstractTableModel *createBaseModel(QString *tableName, bool header) = 0;
    virtual QAbstractItemDelegate *createBaseItemDelegate();
    virtual void resetBaseModel(QAbstractTableModel *baseModel, int logicIndex) = 0;
    virtual bool initUserWidget(int logicsIndex, QWidget *wUserWidget, bool reset) { return false; };
    virtual bool initUserTitleWidget(QWidget *wUserWidget) { return false; };
    virtual int defaultItemHeight();
    virtual int defaultBaseRowWantShow();
    virtual int iconSize();
    virtual int fontPointSize();
    virtual int baseTableDefaultRowHeight();
    virtual int mainHeaderViewHeight() const;
    virtual bool baseTableFirstSectionTextLeftAlign() const;
    virtual bool enableBaseTableHoverItem() const;
    virtual void makeBeauty();
protected:
    void initSync(bool needShowLoading);
    void _resetPos(const QVector<int> &listVisual, bool showImmediately = false);
    void _sortedAsync(bool needShowLoading = false);
    void _resizeEvent();

    //维护可以视范围,不可视范围的数据直接销毁，可以范围的数据创建
    void _updateShouldShowList();
    void hideAllItemSync();
    int _findVisualIndexByPos(int pos, bool nearest = false);
    LocustTableWidgetItem *getItemWidgetByPos(int pos);
    void startTimerUpdateShouldShow();
    LocustTableWidgetItem *getItemWidget(int logicsIndex);
    LocustTableWidgetHeaderView *getHeaderview() const;
    LocustTableWidgetBaseHeaderView *getBaseHeaderview() const;
    void _hBarRangeOrPageChange();
    void hBarRangeOrPageChange();
    void headerViewSectionResized(int logicalIndex, int oldSize, int newSize);
protected:
    friend class LocustTableWidgetHeaderView;
    friend class LocustTableWidgetItem;
    friend class LocustTableWidgetItemView;
    friend class LocustFetchMoreItem;
    friend class LocustGraphicsScene;

    CoroutineGroup *operations;
    QList<LocustTableWidgetItem *> listLogics; // save by logical index

    QVector<int>  listVisuals; //大小可能比 listLogics 小. key是visual Index, value 是logicalIndex
    QVector<int>  sumHeight;

    QGraphicsScene *scene;
    LocustGraphicsView *view;

    QAbstractTableModel *mainModel;
    LocustTableWidgetHeaderView *headerview;

    QScrollBar *hBar;
    
    QVariant subTableFilter;
    QString tipsWhenBaseTableUnvisible;

    int sortColumn;
    Qt::SortOrder order;
    int sortRole;

    int visualBegin;
    int visualEnd;
    int tmUpdateShouldShow;
    int tmResize;
    int tmhBarDelayChange;

    int lastContentHeight;
    int lastExpandVisualIndex;

    LocustFetchMoreItem *moreItem;

    std::set<quint64> selectedItemUserContents;     //key: userContent -> 总是有效(除非不维护 index.data(SrcIndexRole)) 
    std::set<int> logicsSelectedItems;  //key: logicsIndex  ->当发生删除 数据可能无效(数据是往后添加的，所有添加的情况还是有效)
    bool logicsSelectedItemsValid;
    int curSelectedItemVisualIndex;
    bool inMouseMove;
    bool inPress;
    int curPressMouseMoveItemVisualIndex;
};

#endif // LOCUSTTABLEWIDGET_H
