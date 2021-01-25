#ifndef ITEMSELECTIONLISTVIEW_H
#define ITEMSELECTIONLISTVIEW_H

#include <QListView>

class ItemSelectionListView : public QListView
{
    Q_OBJECT
public:
    ItemSelectionListView(QWidget* parent = nullptr) : QListView(parent)
    {}

signals:
    void itemSelectionChanged();

protected:
    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) override
    {
        emit itemSelectionChanged();
        QListView::selectionChanged(selected, deselected);
    }

};

#endif // ITEMSELECTIONLISTVIEW_H
