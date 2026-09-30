package com.yotobe.app;

import android.app.AlertDialog;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.widget.Toast;

public class DownloadHelper {

    public static void showDownloadDialog(MainActivity activity, String currentUrl) {
        if (currentUrl == null || currentUrl.isEmpty()) {
            Toast.makeText(activity, "No video URL to download", Toast.LENGTH_SHORT).show();
            return;
        }

        // Clean mobile URL to standard video URL if needed
        String videoUrl = currentUrl;
        if (videoUrl.contains("m.youtube.com")) {
            videoUrl = videoUrl.replace("m.youtube.com", "www.youtube.com");
        }

        final String finalUrl = videoUrl;

        String[] options = {
            "Copy Video Link",
            "Share Video Link",
            "Open in External Downloader (Seal / NewPipe)",
            "Download via Web Portal (Cobalt)"
        };

        new AlertDialog.Builder(activity)
                .setTitle(R.string.download_title)
                .setItems(options, (dialog, which) -> {
                    switch (which) {
                        case 0: // Copy Link
                            ClipboardManager clipboard = (ClipboardManager) activity.getSystemService(Context.CLIPBOARD_SERVICE);
                            ClipData clip = ClipData.newPlainText("YouTube Video Link", finalUrl);
                            if (clipboard != null) {
                                clipboard.setPrimaryClip(clip);
                                Toast.makeText(activity, R.string.url_copied, Toast.LENGTH_SHORT).show();
                            }
                            break;

                        case 1: // Share Link
                            Intent shareIntent = new Intent(Intent.ACTION_SEND);
                            shareIntent.setType("text/plain");
                            shareIntent.putExtra(Intent.EXTRA_SUBJECT, "YouTube Video");
                            shareIntent.putExtra(Intent.EXTRA_TEXT, finalUrl);
                            activity.startActivity(Intent.createChooser(shareIntent, "Share Video via"));
                            break;

                        case 2: // External Downloader Intent
                            try {
                                Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse(finalUrl));
                                intent.putExtra(Intent.EXTRA_TEXT, finalUrl);
                                activity.startActivity(Intent.createChooser(intent, "Open with Downloader"));
                            } catch (Exception e) {
                                Toast.makeText(activity, "No downloader application found", Toast.LENGTH_SHORT).show();
                            }
                            break;

                        case 3: // Web Portal (Cobalt)
                            try {
                                String cobaltUrl = "https://cobalt.tools/#" + Uri.encode(finalUrl);
                                activity.loadUrl(cobaltUrl);
                            } catch (Exception e) {
                                Toast.makeText(activity, "Failed to open downloader portal", Toast.LENGTH_SHORT).show();
                            }
                            break;
                    }
                })
                .setNegativeButton("Cancel", null)
                .show();
    }
}
