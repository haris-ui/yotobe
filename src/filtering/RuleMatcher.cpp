#include "RuleMatcher.h"

RuleMatcher::RuleMatcher() = default;

// ---------------------------------------------------------------------------
//  Internal helpers
// ---------------------------------------------------------------------------

void RuleMatcher::addRuleUnlocked(const FilterRule& rule) {
    // Caller must hold the write lock.
    if (rule.isException) {
        m_exceptionRules.append(rule);
    } else if (!rule.domainFilter.isEmpty()) {
        m_domainBlockRules[rule.domainFilter.toLower()].append(rule);
    } else {
        m_genericBlockRules.append(rule);
    }
}

// Produce candidate lookup keys for a hostname so we can do direct QHash
// lookups instead of iterating every key.
// "sub.youtube.com" → ["sub.youtube.com", "youtube.com", "com"]
QStringList RuleMatcher::domainCandidates(const QString& host) {
    QStringList candidates;
    candidates.reserve(4);
    candidates.append(host);
    int dot = host.indexOf('.');
    while (dot != -1) {
        candidates.append(host.mid(dot + 1));
        dot = host.indexOf('.', dot + 1);
    }
    return candidates;
}

// ---------------------------------------------------------------------------
//  Public API
// ---------------------------------------------------------------------------

void RuleMatcher::clear() {
    QWriteLocker locker(&m_lock);
    m_exceptionRules.clear();
    m_genericBlockRules.clear();
    m_domainBlockRules.clear();
}

int RuleMatcher::ruleCount() const {
    QReadLocker locker(&m_lock);
    int count = m_exceptionRules.size() + m_genericBlockRules.size();
    for (const auto& list : m_domainBlockRules) {
        count += list.size();
    }
    return count;
}

void RuleMatcher::addRule(const FilterRule& rule) {
    QWriteLocker locker(&m_lock);
    addRuleUnlocked(rule);
}

void RuleMatcher::addRules(const QList<FilterRule>& rules) {
    QWriteLocker locker(&m_lock);  // acquire once for the entire batch
    for (const auto& rule : rules) {
        addRuleUnlocked(rule);
    }
}

// ---------------------------------------------------------------------------
//  Matching predicate helpers
// ---------------------------------------------------------------------------

bool RuleMatcher::matchesResource(const FilterRule& rule, ResourceTypeFlag resourceType) const {
    if (rule.resourceTypes == static_cast<int>(ResourceTypeFlag::Any) ||
        resourceType == ResourceTypeFlag::Any) {
        return true;
    }
    return (rule.resourceTypes & static_cast<int>(resourceType)) != 0;
}

bool RuleMatcher::matchesParty(const FilterRule& rule,
                                const QUrl& targetUrl,
                                const QUrl& firstPartyUrl) const {
    if (!rule.thirdPartyOnly && !rule.firstPartyOnly) return true;

    const QString targetHost     = targetUrl.host().toLower();
    const QString firstPartyHost = firstPartyUrl.host().toLower();

    // Use "." prefix to prevent suffix collisions (e.g. "tube.com" matching "youtube.com")
    const bool isThirdParty =
        !firstPartyHost.isEmpty() &&
        !targetHost.endsWith('.' + firstPartyHost) &&
        targetHost != firstPartyHost &&
        !firstPartyHost.endsWith('.' + targetHost) &&
        firstPartyHost != targetHost;

    if (rule.thirdPartyOnly && !isThirdParty) return false;
    if (rule.firstPartyOnly &&  isThirdParty) return false;
    return true;
}

// ---------------------------------------------------------------------------
//  Core evaluation  (called on the WebEngine IO thread under read lock)
// ---------------------------------------------------------------------------

MatchResult RuleMatcher::evaluate(const QUrl& url,
                                   ResourceTypeFlag resourceType,
                                   const QUrl& firstPartyUrl) const {
    QReadLocker locker(&m_lock);
    MatchResult result;
    const QString urlString = url.toString();
    const QString host      = url.host().toLower();

    // 1. Exception rules always take priority (anti-breakage allowlist).
    for (const auto& rule : m_exceptionRules) {
        if (matchesResource(rule, resourceType) &&
            matchesParty(rule, url, firstPartyUrl) &&
            rule.regex.match(urlString).hasMatch()) {
            result.isException  = true;
            result.shouldBlock  = false;
            result.matchedRule  = rule.rawRule;
            return result;
        }
    }

    // 2. Domain-indexed rules: O(1) hash lookup per candidate domain segment.
    //    Previously this iterated every key in the hash — now we only look up
    //    the exact domain keys that can possibly match this host.
    const QStringList candidates = domainCandidates(host);
    for (const QString& candidate : candidates) {
        auto it = m_domainBlockRules.constFind(candidate);
        if (it == m_domainBlockRules.constEnd()) continue;
        for (const auto& rule : it.value()) {
            if (matchesResource(rule, resourceType) &&
                matchesParty(rule, url, firstPartyUrl) &&
                rule.regex.match(urlString).hasMatch()) {
                result.shouldBlock = true;
                result.matchedRule = rule.rawRule;
                return result;
            }
        }
    }

    // 3. Generic (non-domain-anchored) block rules.
    for (const auto& rule : m_genericBlockRules) {
        if (matchesResource(rule, resourceType) &&
            matchesParty(rule, url, firstPartyUrl) &&
            rule.regex.match(urlString).hasMatch()) {
            result.shouldBlock = true;
            result.matchedRule = rule.rawRule;
            return result;
        }
    }

    result.shouldBlock = false;
    return result;
}
