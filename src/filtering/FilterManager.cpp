#include "FilterManager.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>

FilterManager::FilterManager(QObject* parent)
    : QObject(parent)
    , m_matcher(std::make_shared<RuleMatcher>())
    , m_statistics(std::make_shared<FilterStatistics>(this))
{
    m_interceptor = new UrlFilterInterceptor(m_matcher, m_statistics, this);
}

bool FilterManager::initialize() {
    // Load bundled default YouTube filter list from compiled resource
    QFile resourceFile(":/default_rules/youtube_filters.txt");
    if (resourceFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&resourceFile);
        QString content = in.readAll();
        resourceFile.close();
        return loadRulesFromContent(content);
    }
    qWarning() << "Failed to load default filter rules from Qt resources!";
    return false;
}

bool FilterManager::loadRulesFromFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }
    QTextStream in(&file);
    QString content = in.readAll();
    file.close();
    return loadRulesFromContent(content);
}

bool FilterManager::loadRulesFromContent(const QString& content) {
    QList<FilterRule> rules = RuleParser::parseRuleList(content);
    if (rules.isEmpty()) {
        return false;
    }
    m_matcher->addRules(rules);
    emit rulesUpdated(m_matcher->ruleCount());
    return true;
}

void FilterManager::setFilteringEnabled(bool enabled) {
    if (m_interceptor) {
        m_interceptor->setFilteringEnabled(enabled);
    }
}

bool FilterManager::isFilteringEnabled() const {
    return m_interceptor ? m_interceptor->isFilteringEnabled() : false;
}

int FilterManager::totalRulesLoaded() const {
    return m_matcher ? m_matcher->ruleCount() : 0;
}
