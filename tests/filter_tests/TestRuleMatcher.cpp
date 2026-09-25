#include <cassert>
#include <iostream>
#include <QCoreApplication>
#include "RuleParser.h"
#include "RuleMatcher.h"

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    std::cout << "[Test] Running RuleParser & RuleMatcher tests...\n";

    RuleMatcher matcher;

    // Test 1: Domain blocking rule
    FilterRule rule1;
    bool p1 = RuleParser::parseLine("||googleads.g.doubleclick.net^", rule1);
    assert(p1 && "rule1 should parse successfully");
    matcher.addRule(rule1);

    MatchResult r1 = matcher.evaluate(QUrl("https://googleads.g.doubleclick.net/pagead/ads?client=ca-pub"));
    assert(r1.shouldBlock && "googleads URL should be blocked");
    std::cout << "  Passed: Domain blocking rule\n";

    // Test 2: YouTube ad path rule
    FilterRule rule2;
    bool p2 = RuleParser::parseLine("||youtube.com/api/stats/ads^", rule2);
    assert(p2 && "rule2 should parse successfully");
    matcher.addRule(rule2);

    MatchResult r2 = matcher.evaluate(QUrl("https://www.youtube.com/api/stats/ads?v=xyz"));
    assert(r2.shouldBlock && "YouTube ad stats URL should be blocked");
    std::cout << "  Passed: Path & query matching\n";

    // Test 3: Normal video stream should NOT be blocked
    MatchResult r3 = matcher.evaluate(QUrl("https://www.youtube.com/watch?v=dQw4w9WgXcQ"));
    assert(!r3.shouldBlock && "Normal YouTube watch URL should NOT be blocked");
    std::cout << "  Passed: Legitimate watch URL allowed\n";

    // Test 4: Exception rule (@@) overrides block
    FilterRule ruleBlock;
    RuleParser::parseLine("||youtube.com/videoplayback*", ruleBlock);
    matcher.addRule(ruleBlock);

    FilterRule ruleException;
    RuleParser::parseLine("@@||youtube.com/videoplayback", ruleException);
    matcher.addRule(ruleException);

    MatchResult r4 = matcher.evaluate(QUrl("https://www.youtube.com/videoplayback?expire=123"));
    assert(!r4.shouldBlock && r4.isException && "Exception rule must override block!");
    std::cout << "  Passed: Anti-breakage exception override\n";

    std::cout << "[Test] All RuleMatcher tests passed successfully!\n";
    return 0;
}
