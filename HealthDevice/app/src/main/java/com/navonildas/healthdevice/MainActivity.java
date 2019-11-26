package com.navonildas.healthdevice;

import android.app.Dialog;
import android.content.DialogInterface;
import android.support.v7.app.AlertDialog;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.webkit.WebView;
import android.widget.Button;

import com.android.volley.Request;
import com.android.volley.RequestQueue;
import com.android.volley.Response;
import com.android.volley.VolleyError;
import com.android.volley.toolbox.StringRequest;
import com.android.volley.toolbox.Volley;

public class MainActivity extends AppCompatActivity {
    WebView wv;
    RequestQueue queue;
    StringRequest stringRequest;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
                WindowManager.LayoutParams.FLAG_FULLSCREEN);
        setContentView(R.layout.activity_main);
        wv = findViewById(R.id.webV);
        wv.getSettings().setJavaScriptEnabled(true);
        wv.getSettings().setSupportZoom(false);
        queue = Volley.newRequestQueue(this);
        stringRequest = new StringRequest(Request.Method.GET, "http://192.168.4.1",
                new Response.Listener<String>() {
                    @Override
                    public void onResponse(String response) {
                        wv.loadUrl("http://192.168.4.1");
                    }
                }, new Response.ErrorListener() {
            @Override
            public void onErrorResponse(VolleyError error) {
                AlertDialog a = new AlertDialog.Builder(MainActivity.this).create();
                a.setTitle("Error");
                a.setMessage("Error Device Press Retry");
                a.setButton(Dialog.BUTTON_NEUTRAL, "Retry", new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialogInterface, int i) {
                        queue.add(stringRequest);
                    }
                });
                a.show();
            }
        });
        queue.add(stringRequest);
    }
}
