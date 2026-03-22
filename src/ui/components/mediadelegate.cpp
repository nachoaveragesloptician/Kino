#include "mediadelegate.h"
#include "fileview.h"
#include <QPainter>
#include <QPainterPath>
#include <QFontMetrics>
#include <QLinearGradient>
#include <QStyleOptionViewItem>
#include <QIcon>
#include <QVariant>

MediaDelegate::MediaDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

void MediaDelegate::setCardWidth(int w) { m_cardWidth = w; }
int MediaDelegate::getCardWidth() const { return m_cardWidth; }

QSize MediaDelegate::sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const {
    int height = (m_cardWidth * 3) / 2;
    return QSize(m_cardWidth, height + 15);
}

void MediaDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::SmoothPixmapTransform);

    bool isHovered = option.state & QStyle::State_MouseOver;
    bool isSelected = option.state & QStyle::State_Selected;
    QString title = index.data(Qt::DisplayRole).toString();
    QString subtitle = index.data(SubtitleRole).toString();
    bool isDefaultIcon = index.data(IsDefaultIconRole).toBool();
    QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();

    QString season = index.data(SeasonRole).toString();
    QString episode = index.data(EpisodeRole).toString();
    bool isEpisodeTitle = (!season.isEmpty() && !episode.isEmpty() && title == QString("S%1 E%2").arg(season, episode));

    QRect cardRect = option.rect.adjusted(5, 5, -5, -15);
    int cornerRadius = 10;

    QPainterPath path;
    path.addRoundedRect(cardRect, cornerRadius, cornerRadius);
    painter->setClipPath(path);

    if (isDefaultIcon) {
        painter->fillRect(cardRect, QColor("#1f2335"));
        QPixmap defaultPix = icon.pixmap(QSize(64, 64)); 
        if (!defaultPix.isNull()) {
            QRect iconRect(0, 0, 64, 64);
            iconRect.moveCenter(cardRect.center());
            painter->setOpacity(0.4); 
            painter->drawPixmap(iconRect, defaultPix);
            painter->setOpacity(1.0);
        }
    } else {
        QPixmap pm = icon.pixmap(cardRect.size());
        if (!pm.isNull()) {
            QSize scaledSize = pm.size().scaled(cardRect.size(), Qt::KeepAspectRatioByExpanding);
            QRect pixRect(QPoint(0, 0), scaledSize);
            pixRect.moveCenter(cardRect.center());

            if (!season.isEmpty() && !episode.isEmpty()) {
                QPixmap smallPm = pm.scaled(scaledSize.width() / 4, scaledSize.height() / 4, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                QPixmap blurredPm = smallPm.scaled(scaledSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                painter->drawPixmap(pixRect, blurredPm);
            } else {
                painter->drawPixmap(pixRect, pm);
            }
        }
    }

    if (!season.isEmpty() && !episode.isEmpty()) {
        QString seText = QString("S%1\nE%2").arg(season, episode);
        
        QFont seasonFont = option.font;
        seasonFont.setPixelSize(m_cardWidth / 4.5);
        seasonFont.setWeight(QFont::Black);
        seasonFont.setLetterSpacing(QFont::AbsoluteSpacing, -1);
        painter->setFont(seasonFont);
        
        painter->setPen(QColor(0, 0, 0, 180));
        painter->drawText(cardRect.translated(2, 3), Qt::AlignCenter, seText);
        
        painter->setPen(QColor(255, 255, 255, 245));
        painter->drawText(cardRect, Qt::AlignCenter, seText);
    }

    QRect overlayRect = cardRect;
    overlayRect.setTop(cardRect.bottom() - 65);
    QLinearGradient gradient(overlayRect.topLeft(), overlayRect.bottomLeft());
    gradient.setColorAt(0.0, QColor(26, 27, 38, 0));
    gradient.setColorAt(0.4, QColor(26, 27, 38, 200));
    gradient.setColorAt(1.0, QColor(26, 27, 38, 255));
    painter->fillRect(overlayRect, gradient);

    painter->setClipping(false);

    QRect titleRect = cardRect;
    titleRect.setTop(cardRect.bottom() - 35);
    titleRect.setLeft(cardRect.left() + 10);
    titleRect.setRight(cardRect.right() - 10);

    if (!isEpisodeTitle) {
        QFont titleFont = option.font;
        titleFont.setPixelSize(14);
        titleFont.setBold(true);
        painter->setFont(titleFont);
        painter->setPen(QColor("#ffffff"));

        QFontMetrics fmTitle(titleFont);
        QString elidedTitle = fmTitle.elidedText(title, Qt::ElideRight, cardRect.width() - 20);
        
        painter->drawText(titleRect, Qt::AlignLeft | Qt::AlignTop, elidedTitle);
    }

    if (!subtitle.isEmpty()) {
        QFont subFont = option.font;
        subFont.setPixelSize(11);
        subFont.setBold(true);
        painter->setFont(subFont);
        QFontMetrics fmSub(subFont);
        int subWidth = fmSub.horizontalAdvance(subtitle) + 12;
        int subHeight = fmSub.height() + 4;
        int badgeY = isEpisodeTitle ? cardRect.bottom() - subHeight - 12 : titleRect.top() - subHeight - 4;
        QRect badgeRect(cardRect.left() + 10, badgeY, subWidth, subHeight);
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor("#7aa2f7"));
        painter->drawRoundedRect(badgeRect, subHeight / 2, subHeight / 2);
        painter->setPen(QColor("#1a1b26"));
        painter->drawText(badgeRect, Qt::AlignCenter, subtitle);
    }

    if (isSelected) {
        QColor accentColor("#7aa2f7"); 
        
        painter->setPen(QPen(accentColor, 3)); 
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(cardRect, cornerRadius, cornerRadius);
        
        painter->setPen(QPen(QColor(122, 162, 247, 80), 6)); 
        painter->drawRoundedRect(cardRect, cornerRadius, cornerRadius);
    } else if (isHovered) {
        painter->setPen(QPen(QColor(255, 255, 255, 140), 2));
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(cardRect, cornerRadius, cornerRadius);
    }

    painter->restore();
}