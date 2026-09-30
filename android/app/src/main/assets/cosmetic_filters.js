(function() {
    'use strict';

    // 1. Prevent background playback pause when switching apps or locking screen
    try {
        Object.defineProperty(document, 'hidden', { get: () => false, configurable: true });
        Object.defineProperty(document, 'visibilityState', { get: () => 'visible', configurable: true });
        Object.defineProperty(document, 'webkitHidden', { get: () => false, configurable: true });
        Object.defineProperty(document, 'webkitVisibilityState', { get: () => 'visible', configurable: true });

        const stopVisibilityChange = function(e) {
            e.stopImmediatePropagation();
        };
        window.addEventListener('visibilitychange', stopVisibilityChange, true);
        document.addEventListener('visibilitychange', stopVisibilityChange, true);
    } catch(e) {}

    // 2. Mobile & Desktop YouTube Ad Elimination Styles
    const MOBILE_CSS = `
        /* Feed, Search, and Mobile Ad Units */
        ytm-promoted-sparkles-web-renderer,
        ytm-promoted-video-renderer,
        ytm-companion-ad-renderer,
        ytm-ad-slot-renderer,
        ytm-paid-content-overlay-renderer,
        ytm-mealbar-promo-renderer,
        ytd-ad-slot-renderer,
        ytd-in-feed-ad-layout-renderer,
        ytd-banner-promo-renderer-background,
        ytd-action-companion-ad-renderer,
        ytd-promoted-sparkles-web-renderer,
        ytd-promoted-video-renderer,
        ytd-display-ad-renderer,
        #masthead-ad,
        .ad-container,
        .video-ads,
        .ytp-ad-overlay-container,
        .ytp-ad-message-container,
        .ytp-ad-action-interstitial-background-container,
        .ytp-ad-preview-container,
        .ytp-ad-overlay-slot,
        #player-ads,
        /* App download nags / open in app banners */
        #app-promo-header,
        .c4-tabbed-header-open-app-banner,
        .open-in-app,
        .open-app-button,
        /* Anti-adblock banners */
        tp-yt-paper-dialog:has(#feedback),
        ytd-enforcement-message-view-model,
        #error-screen:has(ytd-enforcement-message-view-model) {
            display: none !important;
            height: 0 !important;
            margin: 0 !important;
            padding: 0 !important;
            pointer-events: none !important;
        }
    `;

    function injectMobileStyles() {
        if (document.getElementById('yotobe-mobile-styles')) return;
        const style = document.createElement('style');
        style.id = 'yotobe-mobile-styles';
        style.textContent = MOBILE_CSS;
        (document.head || document.documentElement).appendChild(style);
    }

    injectMobileStyles();
    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', injectMobileStyles, { once: true });
    }
    window.addEventListener('yt-navigate-finish', injectMobileStyles);

    // 3. Fast video ad skipping
    let isSkipping = false;
    function autoSkipAds() {
        const adShowing = document.querySelector('.ad-showing, .ad-interrupting');
        if (adShowing) {
            const video = document.querySelector('video.html5-main-video') || document.querySelector('video');
            if (video && isFinite(video.duration) && video.duration > 0) {
                if (!isSkipping && (video.duration - video.currentTime > 0.5)) {
                    isSkipping = true;
                    video.currentTime = video.duration;
                }
            }
        } else {
            isSkipping = false;
        }

        const skipBtn = document.querySelector('.ytp-skip-ad-button, .ytp-ad-skip-button, .ytp-ad-skip-button-modern, .ytm-ad-preview-renderer');
        if (skipBtn && typeof skipBtn.click === 'function') {
            skipBtn.click();
        }
    }

    setInterval(autoSkipAds, 500);
})();
