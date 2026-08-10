/*
 * Copyright 2015 Sony Corporation
 */

package com.sonyericsson.idd.api;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.net.Uri;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.RemoteException;

import java.util.List;

/**
 * A service manager for uploading files to the IDD backend
 *
 * You can get an instance of this class by calling
 * {@link #create(Context, IddFileUploadListener)}
 *
 * All methods can be called from any thread.
 * @since 2
 */
public class IddFileUploadManager {


    // Public constants

   /**
    * A status value used in
    * {@link IddFileUploadListener#onStatusUpdated(String, int, int)}
    *
    * Indicates that the IDD service has made a copy of the file and
    * put it in the upload queue. The client can now delete its copy
    * of the file.
    */
    public final static int STATUS_FILE_STAGED               = 0x010;

   /**
    * A status value used in
    * {@link IddFileUploadListener#onStatusUpdated(String, int, int)}
    *
    * Indicates that IDD has started to upload the file
    */
    public final static int STATUS_UPLOAD_STARTED            = 0x020;

   /**
    * A status value used in
    * {@link IddFileUploadListener#onStatusUpdated(String, int, int)}
    *
    * Indicates that the file upload has been delayed and will be uploaded
    * later.
    */
    public final static int STATUS_UPLOAD_DELAYED            = 0x030;

   /**
    * The cause parameter in
    * {@link IddFileUploadListener#onError(String, int)}
    * may be set to this value to indicate that the file upload has
    * permanently failed due to problems to stage the file.
    */
    public final static int ERROR_ON_STAGING = 0x010;

   /**
    * The cause parameter in
    * {@link IddFileUploadListener#onError(String, int)}
    * may be set to this value to indicate that the file upload has
    * permanently failed because the file is too old. IDD deletes staged
    * files that have not been possible to upload within a predefined
    * time limit. (The time limit is typically configured to be several
    * days.)
    */
    public final static int ERROR_TIME_LIMIT_EXPIRED = 0x020;

   /**
    * The cause parameter in
    * {@link IddFileUploadListener#onError(String, int)}
    * may be set to this value to indicate that the file upload has
    * permanently failed due to some kind of reason.
    */
    public final static int ERROR_UNKNOWN = 0x0FF;

    // Public API functions

   /**
    * Creates an IddFileUploadManager. The object is returned in a
    * call to the onFileUploadManagerCreated callback.
    * Can be called on any thread.
    * @param context for the component
    * @param listener object implementing the {@link IddFileUploadListener}
    *        interface.
    */
    static public void create(final Context context, final IddFileUploadListener listener) {
        throw new RuntimeException("Not supported");
    }

   /**
    * Upload file to backend server. The file URI should be generated
    * by a FileProvider and the IDD agent must be granted
    * read access to the file. This is done by granting read
    * access to the package returned by {@link #getIddPackageName()}.
    * The file must be available to IDD until it have been sucessfully staged.
    * See {@link IddFileUploadListener#onStatusUpdated(String, int, int)} and
    * {@link #STATUS_FILE_STAGED}.
    *
    * @param fileUri is a content URI pointing at the file to be uploaded
    * @param fileName is the name the file will get when it is stored
    *        on the backend server.
    * @return uuid for the file upload session
    */
    public String uploadFile(Uri fileUri, String fileName) {
        throw new RuntimeException("Not supported");
    }

   /**
    * Get the name of the IDD package. The package name is used for granting
    * IDD access to files that are uploaded using
    * {@link #uploadFile(Uri, String)}.
    *
    * @return name of package
    */
    public String getIddPackageName() {
        throw new RuntimeException("Not supported");
    }

   /**
    * Get a list with uuids for files staged for upload to server. The
    * files in the list have not been uploaded yet.
    * Can be called on any thread.
    * @return list of uuids
    */
    public List<String> getUuidList() {
        throw new RuntimeException("Not supported");
    }
}
