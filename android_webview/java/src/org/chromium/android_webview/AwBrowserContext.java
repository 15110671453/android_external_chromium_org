// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview;

import android.content.Context;
import android.content.SharedPreferences;

import org.chromium.content.browser.ContentViewStatics;
import org.chromium.net.DefaultAndroidKeyStore;
import org.codeaurora.swe.GeolocationPermissions;

/**
 * Java side of the Browser Context: contains all the java side objects needed to host one
 * browing session (i.e. profile).
 * Note that due to running in single process mode, and limitations on renderer process only
 * being able to use a single browser context, currently there can only be one AwBrowserContext
 * instance, so at this point the class mostly exists for conceptual clarity.
 */
public class AwBrowserContext {

    private static final String HTTP_AUTH_DATABASE_FILE = "http_auth.db";

    private AwCookieManager mCookieManager;
    private AwFormDatabase mFormDatabase;
    private HttpAuthDatabase mHttpAuthDatabase;
    private AwEncryptionHelper mAwEncryptionHelper;
    private DefaultAndroidKeyStore mLocalKeyStore;

    private static AwBrowserContext sAwBrowserContext = null;
    private static SharedPreferences sSharedPreferences = null;

    public static AwBrowserContext getInstance(SharedPreferences sharedPreferences) {
        if (sAwBrowserContext == null) {
            sAwBrowserContext = new AwBrowserContext(sharedPreferences);
        }
        return sAwBrowserContext;
    }

    public AwBrowserContext(SharedPreferences sharedPreferences) {
        sSharedPreferences = sharedPreferences;
    }

    public AwGeolocationPermissions getGeolocationPermissions() {
        return GeolocationPermissions.getInstance(sSharedPreferences);
    }

//SWE-feature-geolocation
    public AwGeolocationPermissions getIncognitoGeolocationPermissions() {
        return GeolocationPermissions.getIncognitoInstance(sSharedPreferences);
    }
//SWE-feature-geolocation

// SWE-feature-username-password
    public void createAwEncryptionHelper(Context context) {
        if (mAwEncryptionHelper == null) {
            mAwEncryptionHelper = new AwEncryptionHelper(context);
        }
    }

    public AwEncryptionHelper getAwEncryptionHelper() {
        return mAwEncryptionHelper;
    }
// SWE-feature-username-password

    public AwQuotaManagerBridge getQuotaManagerBridge(){
        return AwQuotaManagerBridge.getInstance();
    }

    public AwCookieManager getCookieManager() {
        if (mCookieManager == null) {
            mCookieManager = new AwCookieManager();
        }
        return mCookieManager;
    }

    public AwFormDatabase getFormDatabase() {
        if (mFormDatabase == null) {
            mFormDatabase = new AwFormDatabase();
        }
        return mFormDatabase;
    }

    public HttpAuthDatabase getHttpAuthDatabase(Context context) {
        if (mHttpAuthDatabase == null) {
            mHttpAuthDatabase = new HttpAuthDatabase(context, HTTP_AUTH_DATABASE_FILE);
        }
        return mHttpAuthDatabase;
    }

    public DefaultAndroidKeyStore getKeyStore() {
        if (mLocalKeyStore == null) {
            mLocalKeyStore = new DefaultAndroidKeyStore();
        }
        return mLocalKeyStore;
    }

    /**
     * @see android.webkit.WebView#pauseTimers()
     */
    public void pauseTimers() {
        ContentViewStatics.setWebKitSharedTimersSuspended(true);
    }

    /**
     * @see android.webkit.WebView#resumeTimers()
     */
    public void resumeTimers() {
        ContentViewStatics.setWebKitSharedTimersSuspended(false);
    }
}
