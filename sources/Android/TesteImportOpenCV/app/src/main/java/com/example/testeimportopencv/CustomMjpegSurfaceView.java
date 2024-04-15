package com.example.testeimportopencv;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.util.AttributeSet;

import com.github.niqdev.mjpeg.MjpegSurfaceView;

public class CustomMjpegSurfaceView extends MjpegSurfaceView {

    private OnFrameCapturedListener frameCapturedListener;

    public interface OnFrameCapturedListener {
        void onFrameCaptured(Bitmap bitmap);
    }

    public CustomMjpegSurfaceView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public void setOnFrameCapturedListener(OnFrameCapturedListener listener) {
        this.frameCapturedListener = listener;
    }

//    @Override
//    public void draw(Canvas canvas) {
//        super.draw(canvas);
//
//        // Intercept the frame after it's drawn
//        Bitmap bitmap = Bitmap.createBitmap(getWidth(), getHeight(), Bitmap.Config.ARGB_8888);
//        Canvas bitmapCanvas = new Canvas(bitmap);
//        super.draw(bitmapCanvas);
//
//        // Notify listener with the captured frame
//        if (frameCapturedListener != null) {
//            frameCapturedListener.onFrameCaptured(bitmap);
//        }
//    }

    public Bitmap getFrame() {
        setDrawingCacheEnabled(true);

        // Intercept the frame after it's drawn
        Bitmap bitmap = Bitmap.createBitmap(getWidth(), getHeight(), Bitmap.Config.ARGB_8888);
        // Create a canvas with the bitmap
        Canvas canvas = new Canvas(bitmap);

        // Draw the view onto the canvas
        this.draw(canvas);

        // Disable drawing cache
        setDrawingCacheEnabled(false);

        // Return the bitmap
        return bitmap;
    }
}
