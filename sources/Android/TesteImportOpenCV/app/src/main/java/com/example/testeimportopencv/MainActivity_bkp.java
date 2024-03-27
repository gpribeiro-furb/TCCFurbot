package com.example.testeimportopencv;

import android.os.Bundle;
import android.util.Log;
import android.view.SurfaceView;
import android.widget.Toast;

import org.opencv.android.CameraActivity;
import org.opencv.android.CameraBridgeViewBase;
import org.opencv.android.JavaCameraView;
import org.opencv.android.OpenCVLoader;
import org.opencv.core.CvType;
import org.opencv.core.Mat;
import org.opencv.videoio.VideoCapture;

public class MainActivity_bkp extends CameraActivity implements CameraBridgeViewBase.CvCameraViewListener2 {

    private String TAG = "TesteOpenCV";
    private VideoCapture mVideoCapture;
    private JavaCameraView mOpenCvCameraView;
    private Mat mRgba;

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

        // Find JavaCameraView
        mOpenCvCameraView = findViewById(R.id.tutorial1_activity_java_surface_view);
        mOpenCvCameraView.setVisibility(SurfaceView.VISIBLE);
        mOpenCvCameraView.setCvCameraViewListener(this);

        // Create VideoCapture object for the stream
        mVideoCapture = new VideoCapture("http://192.168.1.14:81/stream");
    }

    @Override
    public void onCameraViewStarted(int width, int height) {
        mRgba = new Mat(height, width, CvType.CV_8UC4);
    }

    @Override
    public Mat onCameraFrame(CameraBridgeViewBase.CvCameraViewFrame inputFrame) {
        // Read frame from the video stream
        mVideoCapture.read(mRgba);
        return mRgba;
    }

    @Override
    public void onCameraViewStopped() {
        if (mRgba != null) {
            mRgba.release();
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        // Release resources
        if (mOpenCvCameraView != null) {
            mOpenCvCameraView.disableView();
        }
        if (mVideoCapture != null && mVideoCapture.isOpened()) {
            mVideoCapture.release();
        }
    }
}
