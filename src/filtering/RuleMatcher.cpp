#include "RuleMatcher.h"

RuleMatcher::RuleMatcher() {
}

void RuleMatcher::clear() {
    QWriteLocker locker(&m_lock);
    m_exceptionRules.clear();
    m_genericBlockRules.clear();
    m_domainBlockRules.clear();
}

int RuleMatcher::ruleCount() const {
    QReadLocker locker(&m_lock);
    int count = m_exceptionRules.size() + m_genericBlockRules.size();
    for (auto it = m_domainBlockRules.begin(); it != m_domainBlockRules.end(); ++it) {
        count += it.value().size();
    }
    return count;
}

void RuleMatcher::addRule(const FilterRule& rule) {
    QWriteLocker locker(&m_lock);
    if (rule.isException) {
        m_exceptionRules.append(rule);
    } else if (!rule.domainFilter.isEmpty()) {
        m_domainBlockRules[rule.domainFilter.toLower()].append(rule);
    } else {
        m_genericBlockRules.append(rule);
    }
}

void RuleMatcher::addRules(const QList<FilterRule>& rules) {
    QWriteLocker locker(&m_lock);
    for (const auto& rule : rules) {
        if (rule.isException) {
            m_exceptionRules.append(rule);
        } else if (!rule.domainFilter.isEmpty()) {
            m_domainBlockRules[rule.domainFilter.toLower()].append(rule);
        } else {
            m_genericBlockRules.append(rule);
        }
    }
}

bool RuleMatcher::matchesResource(const FilterRule& rule, ResourceTypeFlag resourceType) const {
    if (rule.resourceTypes == static_cast<int>(ResourceTypeFlag::Any) ||
        resourceType == ResourceTypeFlag::Any) {
        return true;
    }
    return (rule.resourceTypes & static_cast<int>(resourceType)) != 0;
}

bool RuleMatcher::matchesParty(const FilterRule& rule, const QUrl& targetUrl, const QUrl& firstPartyUrl) const {
    if (!rule.thirdPartyOnly && !rule.firstPartyOnly) return true;

    QString targetHost = targetUrl.host().toLower();
    QString firstPartyHost = firstPartyUrl.host().toLower();

    // Use "." prefix to prevent suffix collisions (e.g. "tube.com" matching "youtube.com")
    bool isThirdParty = !firstPartyHost.isEmpty() &&
                        !targetHost.endsWith("." + firstPartyHost) &&
                        targetHost != firstPartyHost &&
                        !firstPartyHost.endsWith("." + targetHost) &&
                        firstPartyHost != targetHost;

    if (rule.thirdPartyOnly && !isThirdParty) return false;
    if (rule.firstPartyOnly && isThirdParty) return false;

    return true;
}

MatchResult RuleMatcher::evaluate(const QUrl& url,
                                  ResourceTypeFlag resourceType,
                                  const QUrl& firstPartyUrl) const {
    QReadLocker locker(&m_lock);
    MatchResult result;
    const QString urlString = url.toString();
    const QString host = url.host().toLower();

    // 1. Check Exception Rules (Anti-Breakage: Exceptions always override blocks)
    for (const auto& rule : m_exceptionRules) {
        if (matchesResource(rule, resourceType) && matchesParty(rule, url, firstPartyUrl)) {
            if (rule.regex.match(urlString).hasMatch()) {
                result.isException = true;
                result.shouldBlock = false;
                result.matchedRule = rule.rawRule;
                return result;
            }
        }
    }

    // 2. Check Domain-Indexed Rules
    for (auto it = m_domainBlockRules.begin(); it != m_domainBlockRules.end(); ++it) {
        const QString& domain = it.key();
        if (host == domain || host.endsWith("." + domain)) {
            for (const auto& rule : it.value()) {
                if (matchesResource(rule, resourceType) && matchesParty(rule, url, firstPartyUrl)) {
                    if (rule.regex.match(urlString).hasMatch()) {
                        result.shouldBlock = true;
                        result.matchedRule = rule.rawRule;
                        return result;
                    }
                }
            }
        }
    }

    // 3. Check Generic Block Rules
    for (const auto& rule : m_genericBlockRules) {
        if (matchesResource(rule, resourceType) && matchesParty(rule, url, firstPartyUrl)) {
            if (rule.regex.match(urlString).hasMatch()) {
                result.shouldBlock = true;
                result.matchedRule = rule.rawRule;
                return result;
            }
        }
    }

    result.shouldBlock = false;
    return result;
}
