package com.example.testeimportopencv;
import android.os.AsyncTask;
import android.util.Log;

import java.io.BufferedOutputStream;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;

public class PostRequestTask extends AsyncTask<String, Void, String> {

    private static final String TAG = "PostRequestTask";
    private PostRequestListener listener;

    public PostRequestTask(PostRequestListener listener) {
        this.listener = listener;
    }

    @Override
    protected String doInBackground(String... params) {
        String urlStr = params[0];
        String postData = params[1];

        HttpURLConnection urlConnection = null;
        BufferedReader reader = null;
        try {
            URL url = new URL(urlStr);
            urlConnection = (HttpURLConnection) url.openConnection();
            urlConnection.setRequestMethod("POST");
            urlConnection.setDoOutput(true);
            urlConnection.setDoInput(true);

            // Write POST data to the connection
            OutputStream out = new BufferedOutputStream(urlConnection.getOutputStream());
            out.write(postData.getBytes());
            out.flush();
            out.close();

            // Read the response
            reader = new BufferedReader(new InputStreamReader(urlConnection.getInputStream()));
            StringBuilder response = new StringBuilder();
            String line;
            while ((line = reader.readLine()) != null) {
                response.append(line).append("\n");
            }
            return response.toString();
        } catch (IOException e) {
            Log.e(TAG, "Error sending POST request: " + e.getMessage());
            return null;
        } finally {
            // Close the connection and reader
            if (urlConnection != null) {
                urlConnection.disconnect();
            }
            if (reader != null) {
                try {
                    reader.close();
                } catch (IOException e) {
                    Log.e(TAG, "Error closing reader: " + e.getMessage());
                }
            }
        }
    }

    @Override
    protected void onPostExecute(String result) {
        if (listener != null) {
            listener.onPostRequestComplete(result);
        }
    }

    public interface PostRequestListener {
        void onPostRequestComplete(String result);
    }
}
