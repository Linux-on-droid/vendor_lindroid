package org.lindroid.ui;

import static org.lindroid.ui.Constants.CONTAINER_NAME;

import android.os.Bundle;

public class SecondaryDisplayActivity extends DisplayActivity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        start(1, CONTAINER_NAME);
    }

}
