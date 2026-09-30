package com.yotobe.app;

import android.annotation.SuppressLint;
import android.app.PictureInPictureParams;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.util.Rational;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.webkit.CookieManager;
import android.webkit.WebChromeClient;
import android.webkit.WebResourceRequest;
import android.webkit.WebResourceResponse;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AlertDialog;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.swiperefreshlayout.widget.SwipeRefreshLayout;

import java.io.ByteArrayInputStream;
import java.io.InputStream;
import java.util.Scanner;

public class MainActivity extends AppCompatActivity {

    private static final String YOUTUBE_HOME_URL = "https://m.youtube.com/";
    // Pure standard Chrome mobile UA (without '; wv') so Google Sign-In succeeds
    private static final String MOBILE_USER_AGENT =
            "Mozilla/5.0 (Linux; Android 14; Pixel 8) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/131.0.6778.200 Mobile Safari/537.36";
    private static final String DESKTOP_USER_AGENT =
            "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/131.0.0.0 Safari/537.36";

    private WebView webView;
    private SwipeRefreshLayout swipeRefreshLayout;
    private ProgressBar loadingProgressBar;
    private LinearLayout bottomBar;
    private FrameLayout fullscreenContainer;

    private View customView;
    private WebChromeClient.CustomViewCallback customViewCallback;
    private boolean isDesktopMode = false;
    private String cosmeticScript = "";

    @Override
    @SuppressLint("SetJavaScriptEnabled")
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        AdBlocker.init(this);
        loadCosmeticScript();

        webView = findViewById(R.id.webView);
        swipeRefreshLayout = findViewById(R.id.swipeRefreshLayout);
        loadingProgressBar = findViewById(R.id.loadingProgressBar);
        bottomBar = findViewById(R.id.bottomBar);
        fullscreenContainer = findViewById(R.id.fullscreenContainer);

        setupWebView();
        setupControls();

        if (savedInstanceState != null) {
            webView.restoreState(savedInstanceState);
        } else {
            webView.loadUrl(YOUTUBE_HOME_URL);
        }
    }

    private void loadCosmeticScript() {
        try {
            InputStream is = getAssets().open("cosmetic_filters.js");
            Scanner scanner = new Scanner(is, "UTF-8").useDelimiter("\\A");
            cosmeticScript = scanner.hasNext() ? scanner.next() : "";
            scanner.close();
            is.close();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    @SuppressLint("SetJavaScriptEnabled")
    private void setupWebView() {
        WebSettings settings = webView.getSettings();
        settings.setJavaScriptEnabled(true);
        settings.setDomStorageEnabled(true);
        settings.setDatabaseEnabled(true);
        settings.setMediaPlaybackRequiresUserGesture(false);
        settings.setAllowFileAccess(false);
        settings.setAllowContentAccess(true);
        settings.setMixedContentMode(WebSettings.MIXED_CONTENT_COMPATIBILITY_MODE);
        settings.setCacheMode(WebSettings.LOAD_DEFAULT);
        settings.setUseWideViewPort(true);
        settings.setLoadWithOverviewMode(true);
        settings.setBuiltInZoomControls(true);
        settings.setDisplayZoomControls(false);

        // Apply clean Chrome Mobile UA
        settings.setUserAgentString(MOBILE_USER_AGENT);

        // Ensure third-party cookies are accepted for Google sign-in
        CookieManager cookieManager = CookieManager.getInstance();
        cookieManager.setAcceptCookie(true);
        cookieManager.setAcceptThirdPartyCookies(webView, true);

        // Swipe Refresh with smart scroll & video drag protection
        swipeRefreshLayout.setColorSchemeResources(R.color.accent_blue);
        swipeRefreshLayout.setProgressBackgroundColorSchemeResource(R.color.bg_surface_elevated);
        swipeRefreshLayout.setOnRefreshListener(() -> webView.reload());

        // CRITICAL FIX: Prevent SwipeRefreshLayout from stealing video drag-to-shrink/minimize gestures!
        swipeRefreshLayout.setOnChildScrollUpCallback((parent, child) -> {
            String currentUrl = webView.getUrl();
            if (isWatchPage(currentUrl)) {
                // When a video is playing, ALWAYS return true so SwipeRefreshLayout NEVER intercepts vertical drag gestures!
                return true;
            }
            return webView.canScrollVertically(-1);
        });

        // WebViewClient
        webView.setWebViewClient(new WebViewClient() {
            @Override
            public WebResourceResponse shouldInterceptRequest(WebView view, WebResourceRequest request) {
                String url = request.getUrl().toString();
                if (AdBlocker.shouldBlock(url)) {
                    return new WebResourceResponse("text/plain", "UTF-8",
                            new ByteArrayInputStream(new byte[0]));
                }
                return super.shouldInterceptRequest(view, request);
            }

            @Override
            public boolean shouldOverrideUrlLoading(WebView view, WebResourceRequest request) {
                Uri uri = request.getUrl();
                String scheme = uri.getScheme();

                if ("http".equalsIgnoreCase(scheme) || "https".equalsIgnoreCase(scheme)) {
                    return false; // let webView load it
                }

                try {
                    Intent intent = new Intent(Intent.ACTION_VIEW, uri);
                    startActivity(intent);
                    return true;
                } catch (Exception e) {
                    return true;
                }
            }

            @Override
            public void doUpdateVisitedHistory(WebView view, String url, boolean isReload) {
                super.doUpdateVisitedHistory(view, url, isReload);
                updateSwipeRefreshState(url);
            }

            @Override
            public void onPageStarted(WebView view, String url, Bitmap favicon) {
                loadingProgressBar.setVisibility(View.VISIBLE);
                updateSwipeRefreshState(url);
            }

            @Override
            public void onPageFinished(WebView view, String url) {
                loadingProgressBar.setVisibility(View.GONE);
                swipeRefreshLayout.setRefreshing(false);
                updateSwipeRefreshState(url);

                // Inject cosmetic adblock & background playback script
                if (!cosmeticScript.isEmpty()) {
                    webView.evaluateJavascript(cosmeticScript, null);
                }
            }
        });

        // WebChromeClient for Fullscreen Video and Progress
        webView.setWebChromeClient(new WebChromeClient() {
            @Override
            public void onProgressChanged(WebView view, int newProgress) {
                loadingProgressBar.setProgress(newProgress);
                if (newProgress >= 100) {
                    loadingProgressBar.setVisibility(View.GONE);
                }
            }

            @Override
            public void onShowCustomView(View view, CustomViewCallback callback) {
                if (customView != null) {
                    callback.onCustomViewHidden();
                    return;
                }

                customView = view;
                customViewCallback = callback;

                bottomBar.setVisibility(View.GONE);
                swipeRefreshLayout.setVisibility(View.GONE);
                fullscreenContainer.addView(view, new FrameLayout.LayoutParams(
                        ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
                fullscreenContainer.setVisibility(View.VISIBLE);

                hideSystemUI();
                setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE);
            }

            @Override
            public void onHideCustomView() {
                if (customView == null) return;

                fullscreenContainer.removeView(customView);
                fullscreenContainer.setVisibility(View.GONE);
                customView = null;

                if (customViewCallback != null) {
                    customViewCallback.onCustomViewHidden();
                    customViewCallback = null;
                }

                bottomBar.setVisibility(View.VISIBLE);
                swipeRefreshLayout.setVisibility(View.VISIBLE);

                showSystemUI();
                setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_USER);
            }
        });
    }

    private void setupControls() {
        ImageButton btnBack = findViewById(R.id.btnBack);
        ImageButton btnForward = findViewById(R.id.btnForward);
        ImageButton btnHome = findViewById(R.id.btnHome);
        ImageButton btnPip = findViewById(R.id.btnPip);
        ImageButton btnDesktop = findViewById(R.id.btnDesktop);
        ImageButton btnDownload = findViewById(R.id.btnDownload);
        ImageButton btnAbout = findViewById(R.id.btnAbout);

        btnBack.setOnClickListener(v -> {
            if (webView.canGoBack()) {
                webView.goBack();
            }
        });

        btnForward.setOnClickListener(v -> {
            if (webView.canGoForward()) {
                webView.goForward();
            }
        });

        btnHome.setOnClickListener(v -> webView.loadUrl(YOUTUBE_HOME_URL));

        btnPip.setOnClickListener(v -> enterPipMode());

        btnDesktop.setOnClickListener(v -> toggleDesktopMode());

        btnDownload.setOnClickListener(v -> {
            String currentUrl = webView.getUrl();
            DownloadHelper.showDownloadDialog(this, currentUrl);
        });

        if (btnAbout != null) {
            btnAbout.setOnClickListener(v -> showAboutDialog());
        }
    }

    private boolean isWatchPage(String url) {
        return url != null && (url.contains("/watch") || url.contains("/shorts/") || url.contains("watch?v="));
    }

    private void updateSwipeRefreshState(String url) {
        if (swipeRefreshLayout == null) return;
        // On video watch/shorts pages, disable pull-to-refresh completely so user can
        // smoothly drag down the video to shrink/minimize it without refreshing the page!
        boolean isVideo = isWatchPage(url);
        swipeRefreshLayout.setEnabled(!isVideo);
    }

    private void showAboutDialog() {
        View dialogView = getLayoutInflater().inflate(R.layout.dialog_about, null);
        AlertDialog dialog = new AlertDialog.Builder(this)
                .setView(dialogView)
                .create();

        if (dialog.getWindow() != null) {
            dialog.getWindow().setBackgroundDrawableResource(android.R.color.transparent);
        }

        View githubLink = dialogView.findViewById(R.id.aboutGithubLink);
        if (githubLink != null) {
            githubLink.setOnClickListener(v -> {
                try {
                    Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse(getString(R.string.github_url)));
                    startActivity(intent);
                } catch (Exception ignored) {}
            });
        }

        View btnClose = dialogView.findViewById(R.id.aboutBtnClose);
        if (btnClose != null) {
            btnClose.setOnClickListener(v -> dialog.dismiss());
        }

        dialog.show();
    }

    private void toggleDesktopMode() {
        isDesktopMode = !isDesktopMode;
        webView.getSettings().setUserAgentString(isDesktopMode ? DESKTOP_USER_AGENT : MOBILE_USER_AGENT);
        Toast.makeText(this, isDesktopMode ? "Desktop Mode: ON" : "Mobile Mode: ON", Toast.LENGTH_SHORT).show();
        webView.reload();
    }

    public void loadUrl(String url) {
        if (webView != null && url != null) {
            webView.loadUrl(url);
        }
    }

    public void enterPipMode() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            try {
                PictureInPictureParams params = new PictureInPictureParams.Builder()
                        .setAspectRatio(new Rational(16, 9))
                        .build();
                enterPictureInPictureMode(params);
            } catch (Exception e) {
                Toast.makeText(this, R.string.pip_not_supported, Toast.LENGTH_SHORT).show();
            }
        } else {
            Toast.makeText(this, R.string.pip_not_supported, Toast.LENGTH_SHORT).show();
        }
    }

    @Override
    protected void onUserLeaveHint() {
        super.onUserLeaveHint();
        // If user presses home while a video is playing, enter PiP smoothly
        String url = webView.getUrl();
        if (url != null && (url.contains("/watch") || url.contains("/shorts/"))) {
            enterPipMode();
        }
    }

    @Override
    public void onPictureInPictureModeChanged(boolean isInPictureInPictureMode, @NonNull Configuration newConfig) {
        super.onPictureInPictureModeChanged(isInPictureInPictureMode, newConfig);
        if (isInPictureInPictureMode) {
            bottomBar.setVisibility(View.GONE);
            loadingProgressBar.setVisibility(View.GONE);
        } else {
            if (customView == null) {
                bottomBar.setVisibility(View.VISIBLE);
            }
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        // Keep webView audio playing in background!
        // Start foreground service to maintain audio pipeline
        PlaybackService.start(this);
    }

    @Override
    protected void onResume() {
        super.onResume();
        PlaybackService.stop(this);
    }

    @Override
    protected void onDestroy() {
        PlaybackService.stop(this);
        if (webView != null) {
            webView.destroy();
        }
        super.onDestroy();
    }

    @Override
    @SuppressLint("MissingSuperCall")
    public void onBackPressed() {
        if (customView != null) {
            if (customViewCallback != null) {
                customViewCallback.onCustomViewHidden();
            }
            return;
        }

        if (webView.canGoBack()) {
            webView.goBack();
            return;
        }

        super.onBackPressed();
    }

    private void hideSystemUI() {
        View decorView = getWindow().getDecorView();
        decorView.setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                        | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                        | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                        | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_FULLSCREEN);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    }

    private void showSystemUI() {
        View decorView = getWindow().getDecorView();
        decorView.setSystemUiVisibility(View.SYSTEM_UI_FLAG_VISIBLE);
        getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    }

    @Override
    protected void onSaveInstanceState(@NonNull Bundle outState) {
        super.onSaveInstanceState(outState);
        webView.saveState(outState);
    }
}
