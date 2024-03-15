package com.example.apptestejava;

import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.Toast;
import androidx.appcompat.app.AppCompatActivity;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Initialize buttons
//        ImageButton button1 = findViewById(R.id.btnUp);
//        ImageButton button2 = findViewById(R.id.btnDown);
//        ImageButton button3 = findViewById(R.id.btnPause);
//        Button button4 = findViewById(R.id.button4);
//        Button button5 = findViewById(R.id.button5);

        // Set click listeners for each button
//        button1.setOnClickListener(view -> showToast("Button 1 clicked"));
//        button2.setOnClickListener(view -> showToast("Button 2 clicked"));
//        button3.setOnClickListener(view -> showToast("Button 3 clicked"));
//        button4.setOnClickListener(view -> showToast("Button 4 clicked"));
//        button5.setOnClickListener(view -> showToast("Button 5 clicked"));
    }

    private void showToast(String message) {
        Toast.makeText(this, message, Toast.LENGTH_SHORT).show();
    }
}
