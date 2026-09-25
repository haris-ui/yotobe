#pragma once

#include "RuleParser.h"
#include <QUrl>
#include <QHash>
#include <QReadWriteLock>

struct MatchResult {
    bool    shouldBlock{false};
    bool    isException{false};
    QString matchedRule;
};

class RuleMatcher {
public:
    RuleMatcher();

    void addRule(const FilterRule& rule);
    void addRules(const QList<FilterRule>& rules);
    void clear();

    int ruleCount() const;

    MatchResult evaluate(const QUrl& url,
                          ResourceTypeFlag resourceType = ResourceTypeFlag::Any,
                          const QUrl& firstPartyUrl = QUrl()) const;

private:
    // Shared dispatch body used by both addRule and addRules (no lock held here).
    void addRuleUnlocked(const FilterRule& rule);

    bool matchesResource(const FilterRule& rule, ResourceTypeFlag resourceType) const;
    bool matchesParty(const FilterRule& rule, const QUrl& targetUrl, const QUrl& firstPartyUrl) const;

    // Extract all candidate domain keys for a given host (e.g. "sub.youtube.com"
    // produces ["sub.youtube.com", "youtube.com", "com"]).  Used for O(1) hash lookups.
    static QStringList domainCandidates(const QString& host);

    mutable QReadWriteLock            m_lock;
    QList<FilterRule>                 m_exceptionRules;
    QList<FilterRule>                 m_genericBlockRules;
    QHash<QString, QList<FilterRule>> m_domainBlockRules;
};
