// FIXME: license file, or use the -l option to generate the files with the header.

package vendor.tms.tmsese_aidl;

@VintfStability
interface ITmsEse {
    // Adding return type to method instead of out param char status since there is only one return value.
    /**
     * Performs an action based on the ioctlType.
     *
     *
     * @param out action performs result.
     */
    int doAction(in long ioctlType);

    // Adding return type to method instead of out param char status since there is only one return value.
    /**
     * @Function seDeInit
     *
     * @Description  This function deinitializes the ESE interface and free all resources.
     *
     * @returns      ESESTATUS_SUCCESS Always return ESESTATUS_SUCCESS (0).
     *
     */
    int sDeinit();

    // Adding return type to method instead of out param byte[] response since there is only one return value.
    /**
     * Returns Answer to Reset as per ISO/IEC 7816
     *
     * @return containing the response. Empty vector if Secure Element
     *                  doesn't support ATR.
     */
    byte[] sGetAtr();

    // Adding return type to method instead of out param char status since there is only one return value.
    /**
     * @Function seInit
     *
     * @Description This function initializes 7816-3-T1 protocol's variables
     *
     * @params      initMode - init mode for normal OMA or download mode
     *
     * @returns     This function return ESESTATUS_SUCCES (0) in case of success.
     *
     */
    int sInit();

    // Adding return type to method instead of out param char status since there is only one return value.
    /**
     * @Function seReset
     *
     * @Description  This function reset the SE, such as: N(S), chain flag, etc.
     *
     * @returns      ESESTATUS_SUCCESS is successful
     *
     */
    int sReset();

    // Adding return type to method instead of out param boolean result since there is only one return value.
    /**
     * Sets Mtk platform spi clk state.
     *
     * @param boolean input true if setting enable, false if setting disable
     * @return as a boolean, true if success, false if failed
     *
     */
    boolean setMtkSpiClk(in boolean enable);

    // Adding return type to method instead of out param byte[] response since there is only one return value.
    /**
     * Transmits an APDU command (as per ISO/IEC 7816) to the SE.
     *
     * @param data APDU command to be sent
     * @return to the command. In case of error in communicating with
     *                  the secure element, an empty vector is returned.
     */
    byte[] transmit(in byte[] data);
}
