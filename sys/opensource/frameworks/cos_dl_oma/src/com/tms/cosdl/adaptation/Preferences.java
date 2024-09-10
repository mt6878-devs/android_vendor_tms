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

package com.tms.cosdl.adaptation;

import java.util.Map;
import java.util.Set;

/**
 * Host system adaptation layer, SharedPreference interface
 *
 * @since 2.0
 */
public interface Preferences {


    /**
     * Save a string type value to preferences
     *
     * @param key   The name of the preference to modify.
     * @param value The new value for the preference.
     * @return Returns a reference to the same Preferences object, so you can
     * chain put calls together.
     */
    Preferences putString(String key, String value);

    /**
     * Save a string set type value to preferences
     *
     * @param key   The name of the preference to modify.
     * @param value The new value for the preference.
     * @return Returns a reference to the same Preferences object, so you can
     * chain put calls together.
     */
    Preferences putStringSet(String key, Set<String> values);

    /**
     * Save a int type value to preferences
     *
     * @param key   The name of the preference to modify.
     * @param value The new value for the preference.
     * @return Returns a reference to the same Preferences object, so you can
     * chain put calls together.
     */
    Preferences putInt(String key, int value);

    /**
     * Save a long type value to preferences
     *
     * @param key   The name of the preference to modify.
     * @param value The new value for the preference.
     * @return Returns a reference to the same Preferences object, so you can
     * chain put calls together.
     */
    Preferences putLong(String key, long value);

    /**
     * Save a float type value to preferences
     *
     * @param key   The name of the preference to modify.
     * @param value The new value for the preference.
     * @return Returns a reference to the same Preferences object, so you can
     * chain put calls together.
     */
    Preferences putFloat(String key, float value);

    /**
     * Save a boolean type value to preferences
     *
     * @param key   The name of the preference to modify.
     * @param value The new value for the preference.
     * @return Returns a reference to the same Preferences object, so you can
     * chain put calls together.
     */
    Preferences putBoolean(String key, boolean value);

    /**
     * deletes the preference value of the specified name
     *
     * @param key The name of the preference to remove.
     * @return Returns a reference to the same Preferences object, so you can
     * chain put calls together.
     */
    Preferences remove(String key);

    /**
     * Clear all preferences
     *
     * @return Returns a reference to the same Preferences object, so you can
     * chain put calls together.
     */
    Preferences clear();

    /**
     * Get all preference values
     *
     * @return Returns a map containing a list of pairs key/value representing
     * the preferences.
     */
    Map<String, ?> getAll();

    /**
     * Get a String value from the preferences.
     *
     * @param key      The name of the preference to retrieve.
     * @param defValue Value to return if this preference does not exist.
     * @return Returns the preference value if it exists, or defValue.
     */

    String getString(String key, String defValue);

    /**
     * Get a set of String values from the preferences.
     *
     * @param key       The name of the preference to retrieve.
     * @param defValues Values to return if this preference does not exist.
     * @return Returns the preference values if they exist, or defValues.
     */
    Set<String> getStringSet(String key, Set<String> defValues);

    /**
     * Get an int value from the preferences.
     *
     * @param key      The name of the preference to retrieve.
     * @param defValue Value to return if this preference does not exist.
     * @return Returns the preference value if it exists, or defValue.
     */
    int getInt(String key, int defValue);

    /**
     * Get a long value from the preferences.
     *
     * @param key      The name of the preference to retrieve.
     * @param defValue Value to return if this preference does not exist.
     * @return Returns the preference value if it exists, or defValue.
     */
    long getLong(String key, long defValue);

    /**
     * Get a float value from the preferences.
     *
     * @param key      The name of the preference to retrieve.
     * @param defValue Value to return if this preference does not exist.
     * @return Returns the preference value if it exists, or defValue.
     */
    float getFloat(String key, float defValue);

    /**
     * Get a boolean value from the preferences.
     *
     * @param key      The name of the preference to retrieve.
     * @param defValue Value to return if this preference does not exist.
     * @return Returns the preference value if it exists, or defValue.
     */
    boolean getBoolean(String key, boolean defValue);

    /**
     * Checks if a preference contains a value with the specified name
     *
     * @param key The name of the preference to check.
     * @return Returns true if the preference exists in the preferences,
     * otherwise false.
     */
    boolean contains(String key);

}
