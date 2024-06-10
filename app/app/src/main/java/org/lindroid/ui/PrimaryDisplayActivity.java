package org.lindroid.ui;

import static org.lindroid.ui.Constants.CONTAINER_NAME;

import android.os.Bundle;

public class PrimaryDisplayActivity extends DisplayActivity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        start(0, CONTAINER_NAME);
    }

}
