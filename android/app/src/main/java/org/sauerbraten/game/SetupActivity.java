package org.sauerbraten.game;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.res.AssetManager;
import android.graphics.Color;
import android.os.Bundle;
import android.util.Log;
import android.view.Gravity;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.TextView;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.List;

/**
 * Launcher: on first run (or after an update) copies the game data packed in the APK's
 * assets/game/ into app storage, where the engine can read it with ordinary file I/O.
 */
public class SetupActivity extends Activity {
    private static final String TAG = "sauerbraten";

    private ProgressBar progress;
    private TextView status;

    static File getBaseDir(Context ctx) {
        File dir = ctx.getExternalFilesDir(null);
        if (dir == null) dir = ctx.getFilesDir();
        return dir;
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        final File gameDir = new File(getBaseDir(this), "game");
        final File stampFile = new File(gameDir, "stamp.txt");
        final String stamp = readAsset("game/stamp.txt");
        if (stamp != null && stamp.equals(readFile(stampFile))) {
            startGame();
            return;
        }

        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setGravity(Gravity.CENTER);
        layout.setBackgroundColor(Color.BLACK);
        layout.setPadding(64, 64, 64, 64);
        status = new TextView(this);
        status.setTextColor(Color.WHITE);
        status.setText("Unpacking game data...");
        progress = new ProgressBar(this, null, android.R.attr.progressBarStyleHorizontal);
        layout.addView(status);
        layout.addView(progress, new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT));
        setContentView(layout);

        new Thread(new Runnable() {
            public void run() {
                try {
                    unpack(gameDir, stampFile, stamp);
                    runOnUiThread(new Runnable() { public void run() { startGame(); } });
                } catch (final IOException e) {
                    Log.e(TAG, "unpacking game data failed", e);
                    runOnUiThread(new Runnable() {
                        public void run() {
                            status.setText("Unpacking game data failed: " + e.getMessage() + "\n\nCheck that there is enough free storage (about 700 MB) and restart.");
                        }
                    });
                }
            }
        }).start();
    }

    private void startGame() {
        startActivity(new Intent(this, SauerActivity.class));
        finish();
    }

    private void unpack(File gameDir, File stampFile, String stamp) throws IOException {
        stampFile.delete();
        AssetManager assets = getAssets();
        List<String> files = new ArrayList<String>();
        BufferedReader list = new BufferedReader(new InputStreamReader(assets.open("game/filelist.txt"), "UTF-8"));
        for (String line; (line = list.readLine()) != null; ) {
            if (line.length() > 0) files.add(line);
        }
        list.close();

        final int total = files.size();
        runOnUiThread(new Runnable() { public void run() { progress.setMax(total); } });
        byte[] buf = new byte[256 * 1024];
        for (int i = 0; i < files.size(); i++) {
            String name = files.get(i);
            File out = new File(gameDir, name);
            File parent = out.getParentFile();
            if (parent != null && !parent.isDirectory() && !parent.mkdirs()) throw new IOException("cannot create " + parent);
            InputStream in = assets.open("game/" + name, AssetManager.ACCESS_STREAMING);
            OutputStream os = new FileOutputStream(out);
            try {
                for (int n; (n = in.read(buf)) > 0; ) os.write(buf, 0, n);
            } finally {
                in.close();
                os.close();
            }
            final int done = i + 1;
            if ((done & 31) == 0 || done == files.size()) {
                runOnUiThread(new Runnable() { public void run() { progress.setProgress(done); } });
            }
        }
        new File(getBaseDir(this), "home").mkdirs();
        OutputStream os = new FileOutputStream(stampFile);
        os.write(stamp.getBytes("UTF-8"));
        os.close();
        Log.i(TAG, "unpacked " + files.size() + " files to " + gameDir);
    }

    private String readAsset(String name) {
        try {
            return readStream(getAssets().open(name));
        } catch (IOException e) {
            return null;
        }
    }

    private static String readFile(File f) {
        try {
            return readStream(new java.io.FileInputStream(f));
        } catch (IOException e) {
            return null;
        }
    }

    private static String readStream(InputStream in) throws IOException {
        StringBuilder sb = new StringBuilder();
        byte[] buf = new byte[4096];
        for (int n; (n = in.read(buf)) > 0; ) sb.append(new String(buf, 0, n, "UTF-8"));
        in.close();
        return sb.toString();
    }
}
