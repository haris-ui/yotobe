#pragma once

#include <QString>
#include <QRegularExpression>
#include <QList>

enum class ResourceTypeFlag {
    Any = 0,
    Script = 1 << 0,
    Image = 1 << 1,
    Subdocument = 1 << 2,
    Media = 1 << 3,
    XHR = 1 << 4,
    Ping = 1 << 5,
    Other = 1 << 6
};

struct FilterRule {
    QString rawRule;
    bool isException{false};      // Starts with @@
    QString domainFilter;         // e.g. youtube.com from ||youtube.com
    QRegularExpression regex;     // Compiled regex for full matching
    int resourceTypes{0};         // Bitmask of ResourceTypeFlag
    bool thirdPartyOnly{false};
    bool firstPartyOnly{false};
};

class RuleParser {
public:
    static bool parseLine(const QString& line, FilterRule& outRule);
    static QList<FilterRule> parseRuleList(const QString& rulesText);

private:
    static QString convertPatternToRegex(const QString& pattern);
    static int parseOptions(const QString& options, FilterRule& rule);
};
