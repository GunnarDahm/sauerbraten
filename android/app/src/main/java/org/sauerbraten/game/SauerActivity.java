package org.sauerbraten.game;

import org.libsdl.app.SDLActivity;

/** Runs the game; SetupActivity has already unpacked the data by the time this starts. */
public class SauerActivity extends SDLActivity {
    @Override
    protected String[] getLibraries() {
        return new String[] { "SDL2", "main" };
    }

    @Override
    protected String[] getArguments() {
        return new String[] { "-A" + SetupActivity.getBaseDir(this).getAbsolutePath() };
    }
}
