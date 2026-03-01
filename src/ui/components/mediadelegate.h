#pragma once

#include <QStyledItemDelegate>

class MediaDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit MediaDelegate(QObject *parent = nullptr);

    void setCardWidth(int w);
    int getCardWidth() const;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    int m_cardWidth = 200;
};