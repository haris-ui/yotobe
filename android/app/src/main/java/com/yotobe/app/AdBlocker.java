package com.yotobe.app;

import android.content.Context;
import android.net.Uri;
import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.HashSet;
import java.util.Set;

public class AdBlocker {
    private static final Set<String> BLOCKED_DOMAINS = new HashSet<>();
    private static final Set<String> BLOCKED_PATTERNS = new HashSet<>();
    private static boolean isInitialized = false;

    public static synchronized void init(Context context) {
        if (isInitialized) return;
        try {
            InputStream is = context.getAssets().open("blocklist.txt");
            BufferedReader reader = new BufferedReader(new InputStreamReader(is));
            String line;
            while ((line = reader.readLine()) != null) {
                line = line.trim();
                if (line.isEmpty() || line.startsWith("#") || line.startsWith("!")) {
                    continue;
                }
                if (line.contains("/")) {
                    BLOCKED_PATTERNS.add(line);
                } else {
                    BLOCKED_DOMAINS.add(line.toLowerCase());
                }
            }
            reader.close();
            isInitialized = true;
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public static boolean shouldBlock(String urlString) {
        if (urlString == null || urlString.isEmpty()) return false;

        Uri uri = Uri.parse(urlString);
        String host = uri.getHost();
        if (host == null) return false;
        host = host.toLowerCase();

        // 1. NEVER block Google Authentication or account login infrastructure
        if (host.equals("accounts.google.com") || host.endsWith(".accounts.google.com") ||
            host.equals("myaccount.google.com") ||
            host.equals("apis.google.com") ||
            host.equals("gstatic.com") || host.endsWith(".gstatic.com") ||
            host.equals("googleapis.com") || host.endsWith(".googleapis.com") ||
            host.equals("recaptcha.net") || host.endsWith(".recaptcha.net") ||
            host.equals("accounts.youtube.com")) {
            return false;
        }

        // 2. Check full domain match
        if (BLOCKED_DOMAINS.contains(host)) {
            return true;
        }

        // Check subdomains
        for (String domain : BLOCKED_DOMAINS) {
            if (host.endsWith("." + domain)) {
                return true;
            }
        }

        // 3. Check pattern substrings (e.g. ad stats, midrolls)
        for (String pattern : BLOCKED_PATTERNS) {
            if (urlString.contains(pattern)) {
                return true;
            }
        }

        return false;
    }
}
