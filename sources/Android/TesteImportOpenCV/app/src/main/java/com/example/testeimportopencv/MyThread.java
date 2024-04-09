package com.example.testeimportopencv;

public class MyThread extends Thread {
    private volatile boolean running = true;

    @Override
    public void run() {
        while (running) {
            // Your thread's task
            // Check if the thread should continue running
            if (!running) {
                return; // Exit the thread if running is set to false
            }

            // Your thread's task code goes here
        }
    }

    // Method to stop the thread
    public void cancel() {
        running = false;
    }
}
