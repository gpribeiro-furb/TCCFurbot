package com.example.testeimportopencv;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;


import com.github.niqdev.mjpeg.DisplayMode;
import com.github.niqdev.mjpeg.Mjpeg;
import com.github.niqdev.mjpeg.MjpegInputStream;
import com.github.niqdev.mjpeg.MjpegInputStreamDefault;
import com.github.niqdev.mjpeg.MjpegSurfaceView;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.ConnectException;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.ArrayList;
import java.util.LinkedList;
import java.util.List;
import java.util.concurrent.TimeUnit;

import org.opencv.android.OpenCVLoader;
import org.opencv.android.Utils;
import org.opencv.core.Mat;
import org.opencv.core.Point;
import org.opencv.core.Scalar;
import org.opencv.imgproc.Imgproc;

import rx.Observable;
import rx.Subscription;
import rx.android.schedulers.AndroidSchedulers;
import rx.schedulers.Schedulers;

public class MainActivity extends AppCompatActivity {

    private String TAG = "TesteOpenCV";
    private ImageView imageView;
    private Handler handler;

//    private String CURRENT_URL = "172.20.10.14";
//    private String CURRENT_URL = "192.168.4.1";
    private String CURRENT_URL = "192.168.1.12";
    private String BASE_URL = "http://" + CURRENT_URL;
    private String IMAGE_URL = BASE_URL + ":81/capture";
    private String STREAM_URL = BASE_URL + ":82/stream";
    private boolean running = false;
    private LinkedList<Bitmap> imagens = new LinkedList<>();
    MjpegSurfaceView mjpegView;
    CustomMjpegSurfaceView customMjpegView;
    Mjpeg mjpeg;
    Subscription subscription;
    private CustomMjpegInputStream stream;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Initialize OpenCV
        if (OpenCVLoader.initLocal()) {
            Log.i(TAG, "OpenCV loaded successfully");
        } else {
            Log.e(TAG, "OpenCV initialization failed!");
            Toast.makeText(this, "OpenCV initialization failed!", Toast.LENGTH_LONG).show();
            return;
        }

        setupButtons();
        setupCampoIp();
        setupVisor();

        connectVisor();
    }

    // Method to fetch image from URL
    private void requestStreamImage() {
//        Thread thread = new Thread(new Runnable() {
//            @Override
//            public void run() {
        try {

            Bitmap teste = customMjpegView.getFrame();
            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    imageView.setImageBitmap(teste);
                }
            });
        } catch (Exception e) {
            Log.e(TAG, "Erro: " + e.getMessage());
        }
//            }
//        });
//        thread.start();
    }

    private void setupVisor() {
        customMjpegView = findViewById(R.id.playerView);
        mjpeg = Mjpeg.newInstance();

        imageView = findViewById(R.id.image_view_teste);
        handler = new Handler();

        handler.postDelayed(imageFetcher, 50);
        handler.postDelayed(imageProcessing, 150);
//        handler.postDelayed(getImageFromStream, 2000);
    }

    private void connectVisor() {
        try {
            subscription = mjpeg.open(STREAM_URL)
                    .subscribe(inputStream -> {
                        customMjpegView.setSource(inputStream);
                        customMjpegView.setDisplayMode(DisplayMode.BEST_FIT);
                        customMjpegView.showFps(true);
                    });
        } catch (Exception e) {
            Log.e(TAG, "Error: Não foi possível conectar a stream no IP: " + STREAM_URL + "\n" + e.getMessage());
        }
    }

    private void setupCampoIp() {
        EditText ipInput = findViewById(R.id.text_ip);
        ipInput.setText(CURRENT_URL);

        // Set up the action listener for the EditText
        ipInput.setOnEditorActionListener(new TextView.OnEditorActionListener() {
            @Override
            public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
                if (actionId == EditorInfo.IME_ACTION_DONE ||
                        (event.getAction() == KeyEvent.ACTION_DOWN &&
                                event.getKeyCode() == KeyEvent.KEYCODE_ENTER)) {
                    // Get the text from the EditText when Enter key is pressed
                    BASE_URL = "http://" + ipInput.getText().toString();
                    STREAM_URL = BASE_URL + ":81/stream";
//                    connectVisor();
                    return true;
                }
                return false;
            }
        });
    }

    private void setupButtons() {
        ImageButton btnUp = findViewById(R.id.btnUp);
        ImageButton btnRight = findViewById(R.id.btnRight);
        ImageButton btnDown = findViewById(R.id.btnDown);
        ImageButton btnLeft = findViewById(R.id.btnLeft);

        btnUp.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                OnClickArrow(ArrowDirection.UP);
            }
        });

        btnRight.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                OnClickArrow(ArrowDirection.RIGHT);
            }
        });

        btnDown.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                OnClickArrow(ArrowDirection.DOWN);
            }
        });

        btnLeft.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                OnClickArrow(ArrowDirection.LEFT);
            }
        });
    }

    private void OnClickArrow(ArrowDirection arrowDirection) {
        String url = BASE_URL + "/test";
        String postData = arrowDirection.name();

        PostRequestTask task = new PostRequestTask(new PostRequestTask.PostRequestListener() {
            @Override
            public void onPostRequestComplete(String result) {
                // Handle the response
                if (result != null) {
                    // Response received successfully
                    Log.d(TAG, "Response: " + result);
                } else {
                    // Error occurred
                    Log.e(TAG, "Error: Failed to receive response");
                }
            }
        });
        task.execute(url, postData);
    }

    // Runnable to fetch image from URL
    private Runnable imageFetcher = new Runnable() {
        @Override
        public void run() {
            fetchImage();
            handler.postDelayed(this, 150);
        }
    };

    // Runnable to fetch image from URL
    private Runnable imageProcessing = new Runnable() {
        @Override
        public void run() {
            processImage();
            handler.postDelayed(this, 50);
        }
    };

    private Runnable getImageFromStream = new Runnable() {
        @Override
        public void run() {
            requestStreamImage();
            handler.postDelayed(this, 2000);
        }
    };

    private void processImage() {
        // Create a new Mat object
        Mat originalImage = new Mat();

        if(!imagens.isEmpty()) {
            Bitmap bitmap = imagens.removeFirst();
            // Convert the Bitmap to Mat
            Utils.bitmapToMat(bitmap, originalImage);

            // Assuming you have already loaded your image into a Mat object (originalImage)
            Mat grayImage = new Mat();
            Imgproc.cvtColor(originalImage, grayImage, Imgproc.COLOR_BGR2GRAY);
            Imgproc.Canny(grayImage, grayImage, 50, 150, 3, false);

            // Detect lines using Hough Line Transform
            Mat lines = new Mat();
            Imgproc.HoughLinesP(grayImage, lines, 1, Math.PI / 180, 25, 50, 30);

            List<int[]> groups = new ArrayList<>();
            List<double[]> linesAux = new ArrayList<>();

            // Draw lines on the original image
            for (int i = 0; i < lines.rows(); i++) {
                double[] vec = lines.get(i, 0);
                double x1 = vec[0], y1 = vec[1], x2 = vec[2], y2 = vec[3];
                double angle = calculateAngle(x1, y1, x2, y2);
                int group;
                if ((-45 >= angle && angle >= -135) || (45 <= angle && angle <= 135)) {
                    group = 0;
                } else {
                    group = 1;
                }
                linesAux.add(new double[]{x1, y1, x2, y2, group});
            }

            if (linesAux != null) {
                for (double[] line : linesAux) {
                    double x1 = line[0];
                    double y1 = line[1];
                    double x2 = line[2];
                    double y2 = line[3];
                    double group = line[4];
                    Point start = new Point(x1, y1);
                    Point end = new Point(x2, y2);
                    // Assuming you have some method colorByGroup(group) to get the color
                    // and drawLine method to draw the line
                    Imgproc.line(originalImage, start, end, colorByGroup(group), 2);
                }
            }

            // Convert the processed Mat image to Bitmap
            Bitmap renderedImage = Bitmap.createBitmap(originalImage.cols(), originalImage.rows(), Bitmap.Config.ARGB_8888);
            Utils.matToBitmap(originalImage, renderedImage);

            // Update UI on the main thread
            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    // Set the fetched image to the ImageView
                    imageView.setImageBitmap(renderedImage);
                }
            });
        }
    }

    // Method to fetch image from URL
    private void fetchImage() {
        Thread thread = new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    long startTime = System.nanoTime();
                    URL url = new URL(IMAGE_URL);
                    HttpURLConnection connection = (HttpURLConnection) url.openConnection();
                    connection.setDoInput(true);
                    connection.connect();
                    InputStream input = connection.getInputStream();
                    final Bitmap bitmap = BitmapFactory.decodeStream(input);
                    imagens.add(bitmap);

                    // Close InputStream
                    input.close();
                } catch (ConnectException e) {
                    Log.e(TAG, "Erro ao conectar com o ESP");
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        });
        thread.start();
    }

    static double calculateAngle(double x1, double y1, double x2, double y2) {
        double angleRadians = Math.atan2(y2 - y1, x2 - x1);
        return Math.toDegrees(angleRadians);
    }

    static Scalar colorByGroup(double group) {
        if (group == 1) {
//            return Color.valueOf(0f,1f,0f); // green
            return new Scalar(0,255,0);
        }
//        return Color.valueOf(0f,0f,1f);// blue
        return new Scalar(255,0,0);
    }

    public enum ArrowDirection {
        UP,
        RIGHT,
        DOWN,
        LEFT
    }
}
