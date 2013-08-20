// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.content.Context;
import android.test.InstrumentationTestCase;
import android.test.suitebuilder.annotation.SmallTest;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.AdvancedMockContext;
import org.chromium.content.common.ProcessInitException;

public class BrowserStartupControllerTest extends InstrumentationTestCase {

    private TestBrowserStartupController mController;

    private static class TestBrowserStartupController extends BrowserStartupController {

        private boolean mThrowProcessInitException;
        private int mStartupResult;
        private boolean mAlreadyInitialized = false;
        private int mInitializedCounter = 0;
        private boolean mCallbackWasSetup;

        private TestBrowserStartupController(Context context) {
            super(context);
        }

        @Override
        void enableAsynchronousStartup() {
            mCallbackWasSetup = true;
        }

        @Override
        boolean initializeAndroidBrowserProcess() throws ProcessInitException {
            mInitializedCounter++;
            if (mThrowProcessInitException) {
                throw new ProcessInitException(4);
            }
            if (!mAlreadyInitialized) {
                // Post to the UI thread to emulate what would happen in a real scenario.
                ThreadUtils.runOnUiThreadBlocking(new Runnable() {
                    @Override
                    public void run() {
                        BrowserStartupController.browserStartupComplete(mStartupResult);
                    }
                });
            }
            return mAlreadyInitialized;
        }

        private boolean hasBeenInitializedOneTime() {
            return mInitializedCounter == 1;
        }
    }

    private static class TestStartupCallback implements BrowserStartupController.StartupCallback {
        private boolean mWasSuccess;
        private boolean mWasFailure;
        private boolean mHasStartupResult;
        private boolean mAlreadyStarted;

        @Override
        public void onSuccess(boolean alreadyStarted) {
            assert !mHasStartupResult;
            mWasSuccess = true;
            mAlreadyStarted = alreadyStarted;
            mHasStartupResult = true;
        }

        @Override
        public void onFailure() {
            assert !mHasStartupResult;
            mWasFailure = true;
            mHasStartupResult = true;
        }
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        Context context = new AdvancedMockContext(getInstrumentation().getTargetContext());
        mController = new TestBrowserStartupController(context);
        // Setting the static singleton instance field enables more correct testing, since it is
        // is possible to call {@link BrowserStartupController#browserStartupComplete(int)} instead
        // of {@link BrowserStartupController#executeEnqueuedCallbacks(int, boolean)} directly.
        BrowserStartupController.overrideInstanceForTest(mController);
    }

    @SmallTest
    public void testSingleAsynchronousStartupRequest() {
        mController.mStartupResult = BrowserStartupController.STARTUP_SUCCESS;
        final TestStartupCallback callback = new TestStartupCallback();

        // Kick off the asynchronous startup request.
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mController.startBrowserProcessesAsync(callback);
            }
        });

        assertTrue("The callback should have been setup", mController.mCallbackWasSetup);
        assertTrue("The browser process should have been initialized one time.",
                mController.hasBeenInitializedOneTime());

        // Wait for callbacks to complete.
        getInstrumentation().waitForIdleSync();

        assertTrue("Callback should have been executed.", callback.mHasStartupResult);
        assertTrue("Callback should have been a success.", callback.mWasSuccess);
        assertFalse("Callback should be told that the browser process was not already started.",
                callback.mAlreadyStarted);
    }

    @SmallTest
    public void testMultipleAsynchronousStartupRequests() {
        mController.mStartupResult = BrowserStartupController.STARTUP_SUCCESS;
        final TestStartupCallback callback1 = new TestStartupCallback();
        final TestStartupCallback callback2 = new TestStartupCallback();
        final TestStartupCallback callback3 = new TestStartupCallback();

        // Kick off the asynchronous startup requests.
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mController.startBrowserProcessesAsync(callback1);
            }
        });
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mController.startBrowserProcessesAsync(callback2);
            }
        });
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mController.addStartupCompletedObserver(callback3);
            }
        });

        assertTrue("The callback should have been setup", mController.mCallbackWasSetup);
        assertTrue("The browser process should have been initialized one time.",
                mController.hasBeenInitializedOneTime());

        // Wait for callbacks to complete.
        getInstrumentation().waitForIdleSync();

        assertTrue("Callback 1 should have been executed.", callback1.mHasStartupResult);
        assertTrue("Callback 1 should have been a success.", callback1.mWasSuccess);
        assertTrue("Callback 2 should have been executed.", callback2.mHasStartupResult);
        assertTrue("Callback 2 should have been a success.", callback2.mWasSuccess);
        assertTrue("Callback 3 should have been executed.", callback3.mHasStartupResult);
        assertTrue("Callback 3 should have been a success.", callback3.mWasSuccess);
        // Some startup tasks might have been enqueued after the browser process was started, but
        // not the first one which kicked of the startup.
        assertFalse("Callback 1 should be told that the browser process was not already started.",
                callback1.mAlreadyStarted);
    }

    @SmallTest
    public void testConsecutiveAsynchronousStartupRequests() {
        mController.mStartupResult = BrowserStartupController.STARTUP_SUCCESS;
        final TestStartupCallback callback1 = new TestStartupCallback();
        final TestStartupCallback callback2 = new TestStartupCallback();

        // Kick off the asynchronous startup requests.
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mController.startBrowserProcessesAsync(callback1);
            }
        });
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mController.addStartupCompletedObserver(callback2);
            }
        });

        assertTrue("The callback should have been setup", mController.mCallbackWasSetup);
        assertTrue("The browser process should have been initialized one time.",
                mController.hasBeenInitializedOneTime());

        // Wait for callbacks to complete.
        getInstrumentation().waitForIdleSync();

        assertTrue("Callback 1 should have been executed.", callback1.mHasStartupResult);
        assertTrue("Callback 1 should have been a success.", callback1.mWasSuccess);
        assertTrue("Callback 2 should have been executed.", callback2.mHasStartupResult);
        assertTrue("Callback 2 should have been a success.", callback2.mWasSuccess);

        final TestStartupCallback callback3 = new TestStartupCallback();
        final TestStartupCallback callback4 = new TestStartupCallback();

        // Kick off more asynchronous startup requests.
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mController.startBrowserProcessesAsync(callback3);
            }
        });
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mController.addStartupCompletedObserver(callback4);
            }
        });

        // Wait for callbacks to complete.
        getInstrumentation().waitForIdleSync();

        assertTrue("Callback 3 should have been executed.", callback3.mHasStartupResult);
        assertTrue("Callback 3 should have been a success.", callback3.mWasSuccess);
        assertTrue("Callback 3 should be told that the browser process was already started.",
                callback3.mAlreadyStarted);
        assertTrue("Callback 4 should have been executed.", callback4.mHasStartupResult);
        assertTrue("Callback 4 should have been a success.", callback4.mWasSuccess);
        assertTrue("Callback 4 should be told that the browser process was already started.",
                callback4.mAlreadyStarted);
    }

    @SmallTest
    public void testSingleFailedAsynchronousStartupRequest() {
        mController.mStartupResult = BrowserStartupController.STARTUP_FAILURE;
        final TestStartupCallback callback = new TestStartupCallback();

        // Kick off the asynchronous startup request.
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mController.startBrowserProcessesAsync(callback);
            }
        });

        assertTrue("The callback should have been setup", mController.mCallbackWasSetup);
        assertTrue("The browser process should have been initialized one time.",
                mController.hasBeenInitializedOneTime());

        // Wait for callbacks to complete.
        getInstrumentation().waitForIdleSync();

        assertTrue("Callback should have been executed.", callback.mHasStartupResult);
        assertTrue("Callback should have been a failure.", callback.mWasFailure);
    }

    @SmallTest
    public void testConsecutiveFailedAsynchronousStartupRequests() {
        mController.mStartupResult = BrowserStartupController.STARTUP_FAILURE;
        final TestStartupCallback callback1 = new TestStartupCallback();
        final TestStartupCallback callback2 = new TestStartupCallback();

        // Kick off the asynchronous startup requests.
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mController.startBrowserProcessesAsync(callback1);
            }
        });
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mController.addStartupCompletedObserver(callback2);
            }
        });

        assertTrue("The callback should have been setup", mController.mCallbackWasSetup);
        assertTrue("The browser process should have been initialized one time.",
                mController.hasBeenInitializedOneTime());

        // Wait for callbacks to complete.
        getInstrumentation().waitForIdleSync();

        assertTrue("Callback 1 should have been executed.", callback1.mHasStartupResult);
        assertTrue("Callback 1 should have been a failure.", callback1.mWasFailure);
        assertTrue("Callback 2 should have been executed.", callback2.mHasStartupResult);
        assertTrue("Callback 2 should have been a failure.", callback2.mWasFailure);

        final TestStartupCallback callback3 = new TestStartupCallback();
        final TestStartupCallback callback4 = new TestStartupCallback();

        // Kick off more asynchronous startup requests.
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mController.startBrowserProcessesAsync(callback3);
            }
        });
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mController.addStartupCompletedObserver(callback4);
            }
        });

        // Wait for callbacks to complete.
        getInstrumentation().waitForIdleSync();

        assertTrue("Callback 3 should have been executed.", callback3.mHasStartupResult);
        assertTrue("Callback 3 should have been a failure.", callback3.mWasFailure);
        assertTrue("Callback 4 should have been executed.", callback4.mHasStartupResult);
        assertTrue("Callback 4 should have been a failure.", callback4.mWasFailure);
    }

    @SmallTest
    public void testAndroidBrowserStartupThrowsException() {
        mController.mThrowProcessInitException = true;
        final TestStartupCallback callback = new TestStartupCallback();

        // Kick off the asynchronous startup request.
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mController.startBrowserProcessesAsync(callback);
            }
        });

        assertTrue("The callback should have been setup", mController.mCallbackWasSetup);
        assertTrue("The browser process should have been initialized one time.",
                mController.hasBeenInitializedOneTime());

        // Wait for callbacks to complete.
        getInstrumentation().waitForIdleSync();

        assertTrue("Callback should have been executed.", callback.mHasStartupResult);
        assertTrue("Callback should have been a failure.", callback.mWasFailure);
    }

    @SmallTest
    public void testAndroidBrowserProcessAlreadyInitializedByOtherPartsOfCode() {
        mController.mAlreadyInitialized = true;
        final TestStartupCallback callback = new TestStartupCallback();

        // Kick off the asynchronous startup request.
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mController.startBrowserProcessesAsync(callback);
            }
        });

        assertTrue("The callback should have been setup", mController.mCallbackWasSetup);
        assertTrue("The browser process should have been initialized one time.",
                mController.hasBeenInitializedOneTime());

        // Wait for callbacks to complete.
        getInstrumentation().waitForIdleSync();

        assertTrue("Callback should have been executed.", callback.mHasStartupResult);
        assertTrue("Callback should have been a success.", callback.mWasSuccess);
        assertTrue("Callback should be told that the browser process was already started.",
                callback.mAlreadyStarted);
    }
}
