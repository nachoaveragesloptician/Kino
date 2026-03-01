#pragma once
#include <QString>
#include <QRegularExpression>

struct MediaInfo {
    QString title;
    QString year;
    bool isSeries = false;
    QString originalPath;
    QString seriesName;
    QString seasonEpisode;
};

class MediaParser {
public:
    static MediaInfo parse(const QString &filename, const QString &path) {
        MediaInfo info;
        info.originalPath = path;
        QString clean = filename;

        int lastDot = clean.lastIndexOf('.');
        if (lastDot > 0) clean = clean.left(lastDot);

        clean.remove(QRegularExpression("\\[.*?\\]"));
        clean.remove(QRegularExpression("\\(.*?\\)"));
        clean.replace('.', ' ');
        clean.replace('_', ' ');

        QRegularExpression seriesRegex("(?i)(.*?)S(\\d+)E(\\d+)");
        QRegularExpressionMatch seriesMatch = seriesRegex.match(clean);
        
        if (seriesMatch.hasMatch()) {
            info.isSeries = true;
            info.seriesName = seriesMatch.captured(1).trimmed();
            info.title = info.seriesName;
            
            info.seasonEpisode = QString("S%1 E%2").arg(seriesMatch.captured(2), seriesMatch.captured(3));
        } else {
            info.isSeries = false;
            QRegularExpression yearRegex("\\b(19|20)\\d{2}\\b");
            QRegularExpressionMatch yearMatch = yearRegex.match(clean);
            if (yearMatch.hasMatch()) {
                info.year = yearMatch.captured(0);
                info.title = clean.left(yearMatch.capturedStart()).trimmed();
            } else {
                info.title = clean.trimmed();
            }
        }
        
        info.title = info.title.simplified();
        info.seriesName = info.seriesName.simplified();
        return info;
    }
};