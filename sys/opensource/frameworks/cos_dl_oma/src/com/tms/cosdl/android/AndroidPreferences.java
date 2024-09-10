/*
 * Copyright (c) Tsingteng MicroSystem Co., Ltd. 2022-2022. All rights reserved.
 */
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

package com.tms.cosdl.android;

import android.content.SharedPreferences;

import com.tms.cosdl.adaptation.Preferences;

import java.util.Map;
import java.util.Set;

/**
 * The implementation of the SharedPreference interface of the host system adaptation layer on the Android system
 *
 * @since 2.0
 */
class AndroidPreferences implements Preferences {

    private final SharedPreferences mPrefs;
    private final SharedPreferences.Editor mEditor;

    AndroidPreferences(android.content.SharedPreferences mPrefs) {
        this.mPrefs = mPrefs;
        this.mEditor = mPrefs.edit();
    }

    @Override
    public Map<String, ?> getAll() {
        return mPrefs.getAll();
    }

    @Override
    public String getString(String key, String defValue) {
        return mPrefs.getString(key, defValue);
    }

    @Override
    public Set<String> getStringSet(String key, Set<String> defValues) {
        return mPrefs.getStringSet(key, defValues);
    }

    @Override
    public int getInt(String key, int defValue) {
        return mPrefs.getInt(key, defValue);
    }

    @Override
    public long getLong(String key, long defValue) {
        return mPrefs.getLong(key, defValue);
    }

    @Override
    public float getFloat(String key, float defValue) {
        return mPrefs.getFloat(key, defValue);
    }

    @Override
    public boolean getBoolean(String key, boolean defValue) {
        return mPrefs.getBoolean(key, defValue);
    }

    @Override
    public boolean contains(String key) {
        return mPrefs.contains(key);
    }

    @Override
    public Preferences putString(String key, String value) {
        mEditor.putString(key, value).commit();
        return this;
    }

    @Override
    public Preferences putStringSet(String key, Set<String> values) {
        mEditor.putStringSet(key, values).commit();
        return this;
    }

    @Override
    public Preferences putInt(String key, int value) {
        mEditor.putInt(key, value).commit();
        return this;
    }

    @Override
    public Preferences putLong(String key, long value) {
        mEditor.putLong(key, value).commit();
        return this;
    }

    @Override
    public Preferences putFloat(String key, float value) {
        mEditor.putFloat(key, value).commit();
        return this;
    }

    @Override
    public Preferences putBoolean(String key, boolean value) {
        mEditor.putBoolean(key, value).commit();
        return this;
    }

    @Override
    public Preferences remove(String key) {
        mEditor.remove(key).commit();
        return this;
    }

    @Override
    public Preferences clear() {
        mEditor.clear().commit();
        return this;
    }

}
