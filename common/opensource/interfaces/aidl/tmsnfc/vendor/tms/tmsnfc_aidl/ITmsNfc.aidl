// FIXME: license file, or use the -l option to generate the files with the header.

package vendor.tms.tmsnfc_aidl;

@VintfStability
interface ITmsNfc {
    // Adding return type to method instead of out param boolean status since there is only one return value.
    /**
     * soft reset the ese
     *
     * @return as a boolean, true if success, false if failed
     */
    boolean EseSoftReset();

    // Adding return type to method instead of out param char status since there is only one return value.
    /**
     * Performs an action based on the ioctlType.
     *
     *
     * @param out action performs result.
     */
    int doAction(in long ioctlType);

    // Adding return type to method instead of out param String value since there is only one return value.
    /**
     * Gets vendor params values whose Key has been provided.
     *
     * @param string
     * @param out output data as string
     */
    String getVendorParam(in String key);

    // Adding return type to method instead of out param boolean result since there is only one return value.
    /**
     * Check if conf file has modified
     *
     * @param uint8_t file type
     * @param out true if file timestamp has modified
     */
    boolean isConfigModified(in byte fileType);

    // Adding return type to method instead of out param boolean status since there is only one return value.
    /**
     * download nfcc fw
     *
     * @param out  status as a int16_t, true if success, false if failed
     */
    boolean nfccFwDownload();

    // Adding return type to method instead of out param boolean status since there is only one return value.
    /**
     * Sets Transit config value
     *
     * @param string transit config value
     * @return as a boolean, true if success, false if failed
     */
    boolean setTmsTransitConfig(in String transitConfValue);
}
