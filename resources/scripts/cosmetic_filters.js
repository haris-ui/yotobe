// Yotobe Cosmetic Filter & Consistency Script
// Ensures clean YouTube ad suppression without breaking site layout, masthead, or sidebar
(function() {
    'use strict';

    // Domain guard: only run on YouTube domains
    const hostname = window.location.hostname;
    if (!hostname.includes('youtube.com') && !hostname.includes('youtu.be')) {
        return;
    }

    const CSS_RULES = `
        /* 1. Feed & Grid Ad Suppression without collapsing layout sections */
        ytd-rich-item-renderer:has(ytd-ad-slot-renderer),
        ytd-rich-item-renderer:has(#ad-content),
        ytd-rich-item-renderer:has([id="ad-content"]),
        ytd-ad-slot-renderer,
        ytd-in-feed-ad-layout-renderer,
        ytd-banner-promo-renderer-background,
        ytd-action-companion-ad-renderer,
        ytd-promoted-sparkles-web-renderer,
        ytd-promoted-video-renderer,
        ytd-display-ad-renderer,
        #masthead-ad {
            display: none !important;
            height: 0 !important;
            margin: 0 !important;
            padding: 0 !important;
        }

        /* 2. Guarantee YouTube top navigation bar and sidebar are never collapsed or hidden */
        ytd-app:not([fullscreen]) #masthead-container {
            display: block !important;
            visibility: visible !important;
            opacity: 1 !important;
        }

        ytd-app:not([fullscreen]) ytd-masthead {
            display: block !important;
            visibility: visible !important;
            opacity: 1 !important;
        }

        ytd-app:not([fullscreen]) #guide,
        ytd-app:not([fullscreen]) #guide-wrapper,
        ytd-app:not([fullscreen]) #guide-content,
        ytd-app:not([fullscreen]) ytd-mini-guide-renderer {
            visibility: visible !important;
            opacity: 1 !important;
        }

        /* 3. Player ad overlays & annotations */
        .ytp-ad-overlay-container,
        .ytp-ad-message-container,
        .ytp-ad-action-interstitial-background-container,
        .ytp-ad-preview-container,
        .ytp-ad-overlay-slot,
        #player-ads,
        #panels ytd-ads-engagement-panel-content-renderer,
        ytd-engagement-panel-section-list-renderer[target-id="engagement-panel-ads"] {
            display: none !important;
            pointer-events: none !important;
        }

        /* 4. Dismiss anti-adblock modals */
        tp-yt-paper-dialog:has(#feedback),
        ytd-enforcement-message-view-model,
        #error-screen:has(ytd-enforcement-message-view-model) {
            display: none !important;
        }
    `;

    function injectStyles() {
        if (document.getElementById('yotobe-cosmetic-styles')) return;
        const style = document.createElement('style');
        style.id = 'yotobe-cosmetic-styles';
        style.textContent = CSS_RULES;
        (document.head || document.documentElement).appendChild(style);
    }

    let skippingAd = false;
    function autoSkipVideoAds() {
        const adContainer = document.querySelector('.ad-showing, .ad-interrupting');
        if (!adContainer) {
            skippingAd = false;
            return;
        }

        const video = document.querySelector('video.html5-main-video');
        if (video && !isNaN(video.duration) && isFinite(video.duration) && video.duration > 0) {
            if (!skippingAd && (video.duration - video.currentTime > 0.5)) {
                skippingAd = true;
                video.currentTime = video.duration;
            }
        }

        const skipButtons = document.querySelectorAll('.ytp-skip-ad-button, .ytp-ad-skip-button, .ytp-ad-skip-button-modern');
        for (let i = 0; i < skipButtons.length; i++) {
            const btn = skipButtons[i];
            if (btn && typeof btn.click === 'function' && btn.offsetParent !== null) {
                btn.click();
            }
        }
    }

    // Apply styles immediately on script load
    injectStyles();

    // Re-apply after DOM is fully ready (handles race on first load)
    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', injectStyles, { once: true });
    }

    // YouTube SPA navigation: 'yt-navigate-finish' fires when YouTube navigates between pages.
    // Use this instead of polling to avoid triggering paint recalculations every 800ms.
    window.addEventListener('yt-navigate-finish', function() {
        // Style element may have been removed by YouTube's Polymer router — re-inject.
        const existing = document.getElementById('yotobe-cosmetic-styles');
        if (existing) existing.remove(); // force re-insert so CSS re-applies to new DOM
        injectStyles();
    });

    // Poll only for ad-skipping (no style injection to prevent layout repaints)
    setInterval(autoSkipVideoAds, 800);
})();

