// Yotobe Cosmetic Filter & Consistency Script
// Ensures clean YouTube ad suppression without broken grid slots, empty gaps, or overlapping elements
(function() {
    'use strict';

    // Domain guard: only run on YouTube domains
    const hostname = window.location.hostname;
    if (!hostname.includes('youtube.com') && !hostname.includes('youtu.be')) {
        return;
    }

    const CSS_RULES = `
        /* 1. Feed & Grid Ad Suppression without breaking grid alignment */
        ytd-rich-item-renderer:has(ytd-ad-slot-renderer),
        ytd-rich-item-renderer:has(#ad-content),
        ytd-rich-item-renderer:has([id="ad-content"]),
        ytd-rich-section-renderer:has(ytd-statement-banner-renderer),
        ytd-rich-section-renderer:has(ytd-brand-video-singleton-renderer),
        ytd-rich-section-renderer:has(ytd-banner-promo-renderer),
        ytd-rich-section-renderer:has(.ytd-in-feed-ad-layout-renderer),
        ytd-ad-slot-renderer,
        ytd-in-feed-ad-layout-renderer,
        ytd-banner-promo-renderer,
        ytd-banner-promo-renderer-background,
        ytd-action-companion-ad-renderer,
        ytd-promoted-sparkles-web-renderer,
        ytd-promoted-video-renderer,
        ytd-display-ad-renderer {
            display: none !important;
        }

        /* 2. Prevent Header & Chips Overlap */
        #masthead-container {
            position: fixed !important;
            top: 0 !important;
            left: 0 !important;
            right: 0 !important;
            z-index: 2020 !important;
            background-color: #0f0f0f !important;
        }

        #chips-wrapper {
            position: sticky !important;
            top: 56px !important;
            z-index: 2010 !important;
            background-color: #0f0f0f !important;
        }

        #masthead-ad {
            display: none !important;
            height: 0 !important;
            margin: 0 !important;
            padding: 0 !important;
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

    function collapseAdGridSlots() {
        // Fallback for browsers / webviews: explicitly collapse the entire parent card
        const adSlots = document.querySelectorAll(
            'ytd-ad-slot-renderer, ytd-in-feed-ad-layout-renderer, ytd-banner-promo-renderer, #masthead-ad'
        );
        for (let i = 0; i < adSlots.length; i++) {
            const parent = adSlots[i].closest('ytd-rich-item-renderer, ytd-rich-section-renderer');
            if (parent && parent.style.display !== 'none') {
                parent.style.setProperty('display', 'none', 'important');
            }
        }
    }

    function autoSkipVideoAds() {
        const video = document.querySelector('video.html5-main-video');
        const adContainer = document.querySelector('.ad-showing, .ad-interrupting');
        
        if (adContainer && video && !isNaN(video.duration) && isFinite(video.duration) && video.duration > 0) {
            video.currentTime = video.duration;
        }

        const skipButtons = document.querySelectorAll('.ytp-skip-ad-button, .ytp-ad-skip-button, .ytp-ad-skip-button-modern');
        for (let i = 0; i < skipButtons.length; i++) {
            const btn = skipButtons[i];
            if (btn && typeof btn.click === 'function') {
                btn.click();
            }
        }
    }

    // Apply styles immediately
    injectStyles();
    collapseAdGridSlots();

    // Debounced observer to prevent layout thrashing and stutter
    let scheduled = false;
    const observer = new MutationObserver(() => {
        if (!scheduled) {
            scheduled = true;
            requestAnimationFrame(() => {
                injectStyles();
                collapseAdGridSlots();
                autoSkipVideoAds();
                scheduled = false;
            });
        }
    });

    observer.observe(document.documentElement, {
        childList: true,
        subtree: true
    });

    setInterval(autoSkipVideoAds, 1000);
})();
