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
    QString seasonNumber;
    QString episodeNumber;
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

        QRegularExpression seriesRegex("(?i)(.*?)\\bS\\s*(\\d+)\\s*E\\s*(\\d+)\\b");
        QRegularExpressionMatch seriesMatch = seriesRegex.match(clean);
        
        if (seriesMatch.hasMatch()) {
            info.isSeries = true;
            info.seriesName = seriesMatch.captured(1).trimmed();
            QRegularExpression trailingYearRegex("\\s+(19|20)\\d{2}$");
            info.seriesName.remove(trailingYearRegex);

            info.title = info.seriesName;
            
            QString s = seriesMatch.captured(2);
            QString e = seriesMatch.captured(3);
            if (s.length() == 1) s = "0" + s;
            if (e.length() == 1) e = "0" + e;
            info.seasonNumber = s;
            info.episodeNumber = e;
            info.seasonEpisode = QString("S%1 E%2").arg(s, e);
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