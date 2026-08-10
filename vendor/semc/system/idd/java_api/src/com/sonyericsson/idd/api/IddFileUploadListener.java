/*
 * Copyright 2015 Sony Corporation
 */

package com.sonyericsson.idd.api;

import android.content.Context; // Needed for Javadoc

/**
 * Interface containing callbacks from the {@link IddFileUploadManager}.
 *
 * The callbacks are invoked on the main thread.
 *
 * An object instance implementing this interface must be provided to the
 * {@link IddFileUploadManager#create(Context, IddFileUploadListener)} method.
 * @since 2
 */
public interface IddFileUploadListener {

   /**
    * Callback that delivers the result from IddFileUploadManager.create
    *
    * @param   mgr     The created IddFileUploadManager
    *
    * @see {@link IddFileUploadManager}.
    */
    void onFileUploadManagerCreated(IddFileUploadManager mgr);

   /**
    * Callback that delivers updates on the file upload progress.
    * Constants for the status and value parameters are defined in
    * {@link IddFileUploadManager}. Note that additional status and
    * value codes may be added in future API versions. Callback
    * implementations should ignore any unknown status and
    * value codes.
    *
    * @param   uuid that identifies the file upload session
    * @param   status code
    * @param   value the usage of the value depends on status code
    *
    * @see {@link IddFileUploadManager}.
    */
    void onStatusUpdated(String uuid, int status, int value);

   /**
    * Callback to inform that the file upload has completed successfully
    *
    * @param   uuid that identifies the file upload session
    *
    * @see {@link IddFileUploadManager}.
    */
    void onCompleted(String uuid);

   /*
    * Callback to inform that the file upload has failed permanently.
    * No more attempts to upload the file will be done.
    *
    * Constants for the value of the cause parameter are defined in
    * {@link IddFileUploadManager}.
    * Note that additional cause values may be added in future API versions.
    * Callback implementations should ignore any unknown causes.
    *
    * @param   uuid that identifies the file upload session
    * @param   cause - Error code indicating the cause of the failure
    *
    * @see {@link IddFileUploadManager}.
    */
    void onError(String uuid, int cause);
}
