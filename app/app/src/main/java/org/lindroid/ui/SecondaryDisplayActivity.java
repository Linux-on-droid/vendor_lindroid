package org.lindroid.ui;

import static android.view.View.SYSTEM_UI_FLAG_FULLSCREEN;
import static org.lindroid.ui.Constants.CONTAINER_NAME;

import android.os.Bundle;

public class SecondaryDisplayActivity extends DisplayActivity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().getDecorView().setSystemUiVisibility(SYSTEM_UI_FLAG_FULLSCREEN);

        start(1, CONTAINER_NAME);
    }

}
