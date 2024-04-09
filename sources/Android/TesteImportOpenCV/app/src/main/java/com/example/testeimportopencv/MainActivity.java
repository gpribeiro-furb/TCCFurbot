package com.example.testeimportopencv;

import android.annotation.SuppressLint;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.text.format.DateUtils;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.VideoView;

import androidx.appcompat.app.AppCompatActivity;
import java.io.IOException;
import java.io.InputStream;
import java.net.ConnectException;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.ArrayList;
import java.util.Date;
import java.util.LinkedList;
import java.util.List;

import org.opencv.android.CameraActivity;
import org.opencv.android.CameraBridgeViewBase;
import org.opencv.android.JavaCameraView;
import org.opencv.android.OpenCVLoader;
import org.opencv.android.Utils;
import org.opencv.core.CvType;
import org.opencv.core.Mat;
import org.opencv.core.Point;
import org.opencv.core.Scalar;
import org.opencv.imgproc.Imgproc;
import org.opencv.videoio.VideoCapture;

public class MainActivity extends AppCompatActivity {

    private String TAG = "TesteOpenCV";
    private ImageView imageView;
    private Handler handler;
//    private static final String IMAGE_URL = "http://192.168.1.14:81/stream"; // Change this to your image URL
    private String BASE_URL = "http://172.20.10.14"; // Change this to your image URL
    private boolean running = false;
    private LinkedList<Bitmap> imagens = new LinkedList<>();


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

        EditText ipInput = findViewById(R.id.text_ip);
        ipInput.setText("172.20.10.14");

        // Set up the action listener for the EditText
        ipInput.setOnEditorActionListener(new TextView.OnEditorActionListener() {
            @Override
            public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
                if (actionId == EditorInfo.IME_ACTION_DONE ||
                        (event.getAction() == KeyEvent.ACTION_DOWN &&
                                event.getKeyCode() == KeyEvent.KEYCODE_ENTER)) {
                    // Get the text from the EditText when Enter key is pressed
                    BASE_URL = "http://" + ipInput.getText().toString();
                    return true;
                }
                return false;
            }
        });

//        VideoCapture videoCapture = new VideoCapture("http://192.168.1.14:81/stream");
//        new Thread(new Runnable() {
//            @Override
//            public void run() {
//                while(true) {
//                    try {
//                        Thread.sleep(500);
//                        Mat mat = new Mat();
//                        boolean teste = videoCapture.read(mat);
//                        // Convert the processed Mat image to Bitmap
//                        Bitmap renderedImage = Bitmap.createBitmap(mat.cols(), mat.rows(), Bitmap.Config.ARGB_8888);
//                        Utils.matToBitmap(mat, renderedImage);
//                        // Update UI on the main thread
//                        runOnUiThread(new Runnable() {
//                            @Override
//                            public void run() {
//                                // Set the fetched image to the ImageView
//                                imageView.setImageBitmap(renderedImage);
//                            }
//                        });
//                    } catch (Exception e) {
//                        Log.e(TAG, e.getMessage());
//                    }
//                }
//            }
//        }).start();


        imageView = findViewById(R.id.image_view_teste);
        handler = new Handler();

        handler.postDelayed(imageFetcher, 500);
        handler.postDelayed(imageProcessing, 50);

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
            handler.postDelayed(this, 500);
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
                    URL url = new URL(BASE_URL + "/capture");
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
