#include "RuleParser.h"
#include <QStringList>

bool RuleParser::parseLine(const QString& line, FilterRule& outRule) {
    QString trimmed = line.trimmed();

    // Ignore empty lines and comments (! or [)
    if (trimmed.isEmpty() || trimmed.startsWith('!') || trimmed.startsWith('[')) {
        return false;
    }

    outRule.rawRule = trimmed;
    outRule.isException = false;
    outRule.resourceTypes = static_cast<int>(ResourceTypeFlag::Any);
    outRule.thirdPartyOnly = false;
    outRule.firstPartyOnly = false;

    // Check exception prefix @@
    if (trimmed.startsWith("@@")) {
        outRule.isException = true;
        trimmed = trimmed.mid(2);
    }

    // Check options after $
    int dollarIdx = trimmed.indexOf('$');
    if (dollarIdx != -1) {
        QString optionsStr = trimmed.mid(dollarIdx + 1);
        trimmed = trimmed.left(dollarIdx);
        parseOptions(optionsStr, outRule);
    }

    // Extract domain if anchored with ||
    if (trimmed.startsWith("||")) {
        QString after = trimmed.mid(2);
        int sepIdx = after.indexOf(QRegularExpression("[\\/\\^\\?\\:]"));
        if (sepIdx != -1) {
            outRule.domainFilter = after.left(sepIdx);
        } else {
            outRule.domainFilter = after;
        }
    }

    QString regexPattern = convertPatternToRegex(trimmed);
    outRule.regex = QRegularExpression(regexPattern, QRegularExpression::CaseInsensitiveOption);

    return outRule.regex.isValid();
}

QList<FilterRule> RuleParser::parseRuleList(const QString& rulesText) {
    QList<FilterRule> rules;
    const QStringList lines = rulesText.split(QRegularExpression("[\r\n]+"), Qt::SkipEmptyParts);
    for (const QString& line : lines) {
        FilterRule rule;
        if (parseLine(line, rule)) {
            rules.append(rule);
        }
    }
    return rules;
}

QString RuleParser::convertPatternToRegex(const QString& pattern) {
    QString str = pattern;
    bool matchStart = false;
    bool matchEnd = false;

    if (str.startsWith("||")) {
        // Domain anchor: matches protocol and domain start
        str = str.mid(2);
        // Regex for protocol + optional subdomains
        str = "^https?:\\/\\/([a-z0-9-_.]+\\.)?" + QRegularExpression::escape(str);
        // Replace escaped ^ with separator regex
        str.replace(QRegularExpression::escape("^"), "([\\/\\?\\:\\^]|$)");
        // Replace escaped * with wildcard regex
        str.replace(QRegularExpression::escape("*"), ".*");
        return str;
    }

    if (str.startsWith('|')) {
        matchStart = true;
        str = str.mid(1);
    }
    if (str.endsWith('|')) {
        matchEnd = true;
        str = str.left(str.length() - 1);
    }

    QString escaped = QRegularExpression::escape(str);
    escaped.replace(QRegularExpression::escape("^"), "([\\/\\?\\:\\^]|$)");
    escaped.replace(QRegularExpression::escape("*"), ".*");

    QString result;
    if (matchStart) result += "^";
    result += escaped;
    if (matchEnd) result += "$";

    return result;
}

int RuleParser::parseOptions(const QString& options, FilterRule& rule) {
    const QStringList optList = options.split(',', Qt::SkipEmptyParts);
    int flags = 0;

    for (const QString& opt : optList) {
        QString lower = opt.trimmed().toLower();
        if (lower == "script") flags |= static_cast<int>(ResourceTypeFlag::Script);
        else if (lower == "image") flags |= static_cast<int>(ResourceTypeFlag::Image);
        else if (lower == "subdocument") flags |= static_cast<int>(ResourceTypeFlag::Subdocument);
        else if (lower == "media") flags |= static_cast<int>(ResourceTypeFlag::Media);
        else if (lower == "xhr" || lower == "xmlhttprequest") flags |= static_cast<int>(ResourceTypeFlag::XHR);
        else if (lower == "ping") flags |= static_cast<int>(ResourceTypeFlag::Ping);
        else if (lower == "third-party") rule.thirdPartyOnly = true;
        else if (lower == "~third-party" || lower == "first-party") rule.firstPartyOnly = true;
    }

    if (flags != 0) {
        rule.resourceTypes = flags;
    }

    return flags;
}
