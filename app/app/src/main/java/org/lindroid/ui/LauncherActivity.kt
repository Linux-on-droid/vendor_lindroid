package org.lindroid.ui

import android.app.Activity
import android.app.ActivityOptions
import android.app.WindowConfiguration
import android.content.DialogInterface
import android.content.Intent
import android.hardware.display.DisplayManager
import android.net.Uri
import android.os.Bundle
import android.util.Log
import android.view.Display
import android.view.LayoutInflater
import android.view.ViewGroup.MarginLayoutParams
import android.view.WindowManager
import android.widget.TextView
import android.widget.Toast
import androidx.core.view.ViewCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.updateLayoutParams
import androidx.recyclerview.widget.RecyclerView
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.io.File
import java.io.FileOutputStream
import java.io.IOException
import java.net.HttpURLConnection
import java.net.URL
import java.util.concurrent.atomic.AtomicBoolean

class LauncherActivity : Activity() {
	private val scope = CoroutineScope(Dispatchers.Default)
	private var mNewName: String? = null

	override fun onCreate(savedInstanceState: Bundle?) {
		super.onCreate(savedInstanceState)
		scope.launch {
			if (!ContainerManager.isPerspectiveAvailable()) {
				withContext(Dispatchers.Main) {
					MaterialAlertDialogBuilder(this@LauncherActivity)
						.setTitle(R.string.unsupported_title)
						.setMessage(R.string.unsupported_message)
						.setPositiveButton(android.R.string.ok) { dialog, _ ->
							dialog.dismiss()
							finish()
						}
						.setIcon(R.drawable.ic_warning)
						.show()
				}
			} else {
				val runningContainer = ContainerManager.isAtLeastOneRunning()
				withContext(Dispatchers.Main) {
					setContentView(R.layout.launcher)
					ViewCompat.setOnApplyWindowInsetsListener(requireViewById<RecyclerView>(R.id.recycler)) { v, windowInsets ->
						val insets = windowInsets.getInsets(WindowInsetsCompat.Type.statusBars())
						v.updateLayoutParams<MarginLayoutParams> {
							topMargin = insets.top
						}
						WindowInsetsCompat.CONSUMED
					}
					if (runningContainer != null) {
						startDisplayActivitiesOnAllDisplays(runningContainer)
					} else {
						withContext(Dispatchers.Default) {
							updateAdapter()
						}
					}
				}
			}
		}
	}

	private suspend fun updateAdapter() {
		val containers = ContainerManager.listContainers()
		val running = containers.map { ContainerManager.isRunning(it) }
		withContext(Dispatchers.Main) {
			requireViewById<RecyclerView>(R.id.recycler).adapter = ContainerAdapter(
				this@LauncherActivity,
				containers to running,
				startContainer = {
					scope.launch {
						startAndOpenDisplayActivitiesOrError(it)
					}
				},
				stopContainer = {
					scope.launch {
						if (ContainerManager.stop(it)) updateAdapter()
					}
				},
				createContainer = {
					scope.launch {
						maybeCreateContainerWithName(it)
					}
				},
				deleteContainer = {
					val v = LayoutInflater.from(this@LauncherActivity)
						.inflate(R.layout.progressdialog, null)
					v.requireViewById<TextView>(R.id.prog_message)
						.setText(R.string.creating_container_message)
					val inner = MaterialAlertDialogBuilder(this@LauncherActivity)
						.setCancelable(false)
						.setTitle(R.string.deleting_container_title)
						.setView(v)
						.show()
					scope.launch {
						val ok = ContainerManager.deleteContainer(it)
						if (ok) updateAdapter()
						withContext(Dispatchers.Main) {
							inner.dismiss()
							if (!ok) {
								MaterialAlertDialogBuilder(this@LauncherActivity)
									.setTitle(R.string.delete_failed)
									.setMessage(R.string.delete_failed_msg)
									.setPositiveButton(android.R.string.ok) { _, _ -> }
									.setCancelable(false)
									.setIcon(R.drawable.ic_warning)
									.show()
							}
						}
					}
				}
			)
		}
	}

	override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
		if (requestCode == SAF_ROOTFS_REQUEST) {
			if (resultCode == RESULT_OK && data != null && data.data != null) {
				val name = mNewName
				scope.launch {
					createContainer(name, data.data)
				}
			} else {
				Toast.makeText(this, R.string.saf_pick_failed, Toast.LENGTH_LONG).show()
			}
			mNewName = null
			return
		}
		super.onActivityResult(requestCode, resultCode, data)
	}

	private suspend fun maybeCreateContainerWithName(name: String) {
		if (!ContainerManager.listContainers().contains(name)) {
			withContext(Dispatchers.Main) {
				MaterialAlertDialogBuilder(this@LauncherActivity)
					.setTitle(R.string.large_download)
					.setMessage(R.string.large_download_msg)
					.setPositiveButton(android.R.string.ok) { _, _ ->
						scope.launch {
							downloadAndCreateContainer(name)
						}
					}
					.setNegativeButton(android.R.string.cancel) { _, _ -> }
					.setNeutralButton(R.string.use_local_file) { _, _ ->
						mNewName = name
						startActivityForResult(Intent(Intent.ACTION_OPEN_DOCUMENT).apply {
							addCategory(Intent.CATEGORY_OPENABLE)
							setType("*/*")
						}, SAF_ROOTFS_REQUEST)
					}
					.setIcon(R.drawable.ic_warning)
					.show()
			}
		} else {
			withContext(Dispatchers.Main) {
				MaterialAlertDialogBuilder(this@LauncherActivity)
					.setTitle(R.string.duplicate_name)
					.setMessage(R.string.duplicate_name_msg)
					.setPositiveButton(android.R.string.ok) { _, _ -> }
					.setCancelable(false)
					.setIcon(R.drawable.ic_warning)
					.show()
			}
		}
	}

	private suspend fun createContainer(containerName: String?, rootfs: Uri?) {
		val inner = withContext(Dispatchers.Main) {
			window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
			val v = LayoutInflater.from(this@LauncherActivity).inflate(R.layout.progressdialog, null)
			v.requireViewById<TextView>(R.id.prog_message).setText(R.string.creating_container_message)
			MaterialAlertDialogBuilder(this@LauncherActivity)
				.setCancelable(false)
				.setTitle(R.string.creating_container_title)
				.setView(v)
				.show()
		}
		val ok = ContainerManager.addContainer(containerName, contentResolver, rootfs)
		withContext(Dispatchers.Main) {
			window.clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
			inner.dismiss()
		}
		if (ok) {
			updateAdapter()
			startAndOpenDisplayActivitiesOrError(containerName)
		} else {
			withContext(Dispatchers.Main) {
				MaterialAlertDialogBuilder(this@LauncherActivity)
					.setTitle(R.string.failed_to_create)
					.setMessage(R.string.failed_to_create_msg)
					.setPositiveButton(android.R.string.ok) { _, _ -> }
					.setCancelable(false)
					.setIcon(R.drawable.ic_warning)
					.show()
			}
		}
	}

	private suspend fun downloadAndCreateContainer(containerName: String) {
		// TODO download in background (service?)... in general, move creation to service
		val cancel = AtomicBoolean(false)
		val (tv, inner) = withContext(Dispatchers.Main) {
			window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
			val v = LayoutInflater.from(this@LauncherActivity).inflate(R.layout.progressdialog, null)
			val tv = v.requireViewById<TextView>(R.id.prog_message)
			tv.setText(R.string.dl_connecting)
			tv to MaterialAlertDialogBuilder(this@LauncherActivity)
				.setCancelable(false)
				.setTitle(R.string.downloading)
				.setNegativeButton(R.string.cancel) { _, _ -> cancel.set(true) }
				.setView(v)
				.show()
		}
		val rootfs = Constants.getDownloadUriForCurrentArch()
		var f: File? = null
		try {
			val conn = URL(rootfs.toString()).openConnection() as HttpURLConnection
			val lm = conn.lastModified.let { if (it == 0L) System.currentTimeMillis() else it }
			f = File(cacheDir, "rootfs_$lm")
			if (f.exists()) { // yay, cache hit
				conn.inputStream.close()
			} else {
				val contentLength = conn.contentLengthLong
				var readBytes: Long = 0
				var lastReportedReadBytes: Long = 0
				val b = ByteArray(16 * 1024 * 1024)
				conn.inputStream.use { s ->
					FileOutputStream(f).use { s2 ->
						var r: Int
						while ((s.read(b).also { r = it }) != -1 && !cancel.get()) {
							s2.write(b, 0, r)
							if (r >= 0) readBytes += r.toLong()
							if (lastReportedReadBytes < readBytes - 1024 * 1024) {
								lastReportedReadBytes = readBytes
								val rb = readBytes
								scope.launch(Dispatchers.Main) {
									tv.text = getString(
										R.string.progress,
										rb / (1024 * 1024),
										contentLength / (1024 * 1024)
									)
								}
							}
						}
					}
				}
				if (cancel.get()) {
					if (f.exists() && !f.delete())
						Log.e(TAG, "failed to delete temp file")
					return
				}
			}
		} catch (e: IOException) {
			Log.e(TAG, Log.getStackTraceString(e))
			if (f != null && f.exists() && !f.delete())
				Log.e(TAG, "failed to delete temp file")
			withContext(Dispatchers.Main) {
				Toast.makeText(
					this@LauncherActivity,
					R.string.download_failed,
					Toast.LENGTH_LONG
				).show()
			}
			return
		} finally {
			withContext(Dispatchers.Main) {
				window.clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
				inner.dismiss()
			}
		}
		createContainer(containerName, Uri.fromFile(f))
	}

	private suspend fun startAndOpenDisplayActivitiesOrError(containerName: String?) {
		val ok = ContainerManager.start(containerName)
		withContext(Dispatchers.Main) {
			if (ok) {
				startDisplayActivitiesOnAllDisplays(containerName)
			} else {
				MaterialAlertDialogBuilder(this@LauncherActivity)
					.setTitle(R.string.failed_to_start)
					.setMessage(R.string.failed_to_start_msg)
					.setPositiveButton(android.R.string.ok) { _, _ -> }
					.setCancelable(false)
					.setIcon(R.drawable.ic_warning)
					.show()
			}
		}
	}

	private fun startDisplayActivitiesOnAllDisplays(containerName: String?) {
		val displayManager = getSystemService(DISPLAY_SERVICE) as DisplayManager
		val displays = displayManager.displays

		// int currentId = Objects.requireNonNull(getDisplay()).getDisplayId();

		// TODO don't do this on foldable's other internal displays
		for (display in displays) {
			if ((display.flags and Display.FLAG_PRIVATE) != 0) continue
			val displayId = display.displayId
			Log.d(
				TAG,
				"Starting DisplayActivity on display: $displayId"
			)
			val intent = Intent(
				this,
				DisplayActivity::class.java
			)
			intent.putExtra("displayID", displayId)
			intent.putExtra("containerName", containerName)
			intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK or Intent.FLAG_ACTIVITY_MULTIPLE_TASK or Intent.FLAG_ACTIVITY_NEW_DOCUMENT)
			val options = ActivityOptions.makeBasic()
			options.setLaunchDisplayId(displayId)
			options.setLaunchWindowingMode(WindowConfiguration.WINDOWING_MODE_FULLSCREEN)

			startActivity(intent, options.toBundle())
		}
		finish()
	}

	companion object {
		private const val TAG = "LauncherActivity"
		private const val SAF_ROOTFS_REQUEST = 1000
	}
}
