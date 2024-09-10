/*
 * Copyright (C) 2022 Tsingteng MicroSystem
 *
 * All rights are reserved. Reproduction in whole or in part is
 * prohibited without the written consent of the copyright owner.
 *
 * Tsingteng reserves the right to make changes without notice at any time.
 *
 * Tsingteng makes no warranty, expressed, implied or statutory, including but
 * not limited to any implied warranty of merchantability or fitness for any
 * particular purpose, or that the use will not infringe any third party patent,
 * copyright or trademark. Tsingteng must not be liable for any loss or damage
 * arising from its use.
 */
package com.tms.sems

import android.content.Context
import android.se.omapi.Channel
import android.se.omapi.SEService
import android.se.omapi.Session
import android.util.Log
import java.util.Locale
import java.util.concurrent.Executors

object TmsSemsManager {

    private const val TAG = "TmsSemsManager"

    const val RESULT_SUCCESS = 0
    const val RESULT_SCRIPT_ERROR = -1
    const val RESULT_OMA_ERROR = -2
    const val RESULT_EXEC_ERROR = -3
    const val RESULT_APDU_ERROR = -4

    private var mSEService: SEService? = null
    private var mChannel: Channel? = null

    var mUseReaderName = "eSE1"
    var mSuccessResponse = "9000"

    fun changeConfig(t: (TmsSemsManager) -> Unit) {
        t.invoke(this)
    }

    fun execSemsScript(context: Context, script: String, callback: ITmsSemsCallback): Boolean {
        val scriptLines = script.lines().mapIndexed { i, s -> i to s.trim() }
            .filterNot { it.second.isEmpty() || it.second.startsWith("#") || it.second.startsWith(";") }
        if (scriptLines.isEmpty()) {
            Log.e(TAG, "empty script")
            return false
        }
        mSEService = SEService(context, Executors.newSingleThreadExecutor(), object : SEService.OnConnectedListener {
            override fun onConnected() {
                Log.i(TAG, "SEService connected")
                val seService = mSEService ?: return
                val startTime = System.currentTimeMillis()
                val resultCode = execScriptUseOma(seService, scriptLines)
                val useTime = System.currentTimeMillis() - startTime
                if (resultCode != RESULT_SUCCESS) {
                    Log.e(TAG, "script exec failed, code = $resultCode, use time = ${useTime}ms")
                } else {
                    Log.i(TAG, "script exec success, use time = ${useTime}ms")
                }
                mSEService?.shutdown()
                mChannel = null
                mSEService = null
                callback.onSemsScriptExecComplete(resultCode)
            }
        })
        return true
    }

    private fun execScriptUseOma(
        seService: SEService, scriptLines: List<Pair<Int, String>>
    ): Int {
        val reader = seService.readers.find { it.name == mUseReaderName }
        if (reader == null) {
            Log.i(TAG, "$mUseReaderName not found")
            return RESULT_OMA_ERROR
        }
        try {
            val session = reader.openSession()
            for ((lineNumber, line) in scriptLines) {
                try {
                    Log.d(TAG, "send: $line")
                    val rsp = execApdu(session, line)
                    Log.d(TAG, "recv: $rsp")
                    if (rsp == null) {
                        Log.e(TAG, "exec line($lineNumber) failed, $line")
                        return RESULT_EXEC_ERROR
                    } else if (rsp.endsWith(mSuccessResponse).not()) {
                        if (line.startsWith("80E40080") && rsp.endsWith("6A88")) {
                            Log.d(TAG, "ignore error code for deleting non-existent apps")
                            continue
                        } else if (line.startsWith("80EA0100") && rsp.endsWith("6900")) {
                            Log.d(TAG, "ignore 6900 for 80EA")
                            continue
                        }
                        Log.e(TAG, "exec line($lineNumber) status code check failed, $line")
                        return RESULT_APDU_ERROR
                    }
                } catch (e: Exception) {
                    Log.e(TAG, "parse line($lineNumber) failed, $line", e)
                    return RESULT_SCRIPT_ERROR
                }
            }
        } catch (e: Exception) {
            Log.e(TAG, "OMA operation abnormal", e)
            return RESULT_OMA_ERROR
        }
        return RESULT_SUCCESS
    }

    private fun execApdu(session: Session, line: String): String? {
        if (line.startsWith("00A40400", ignoreCase = true)) {
            val aid = line.substring(10)
            Log.d(TAG, "select aid: $aid")
            if (mChannel != null) {
                mChannel?.close()
            }
            mChannel = session.openBasicChannelSafety(aid.toHex())
            return mChannel?.selectResponse?.toHex()
        } else {
            val channel = mChannel ?: return null
            return channel.transmitSafety(line.toHex()).toHex()
        }
    }

    private fun Session.openBasicChannelSafety(aid: ByteArray): Channel? {
        return try {
            openBasicChannel(aid)
        } catch (e: Exception) {
            Log.e(TAG, "openBasicChannel failed", e)
            null
        }
    }

    private fun Channel.transmitSafety(data: ByteArray): ByteArray? {
        return try {
            transmit(data)
        } catch (e: Exception) {
            Log.e(TAG, "transmit failed", e)
            null
        }
    }

    private fun String.toHex(): ByteArray {
        return chunked(2).map { it.toInt(16).toByte() }.toByteArray()
    }

    private fun ByteArray?.toHex(): String {
        return this?.joinToString(separator = "") {
            (it.toInt() and 0xFF).toString(16).toUpperCase(Locale.ROOT).let { s ->
                if (s.length == 1) "0$s" else s
            }
        } ?: ""
    }
}