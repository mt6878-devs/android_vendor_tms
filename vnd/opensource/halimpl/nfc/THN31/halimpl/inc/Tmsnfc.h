/******************************************************************************
 * Copyright (c) 2021-2022 Tsingteng MicroSystem
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
 *****************************************************************************/

/*******************************************************************************
**
** Function         phTmsNciHalGetSystemProperty
**
** Description      It shall be used to get property value of the given Key
**
** Parameters       string key
**
** Returns          It returns the property value of the key
*******************************************************************************/
std::string phTmsNciHalGetSystemProperty(std::string key);

/******************************************************************************
** Function         phTmsNciHalSetTmsTransitConfig
**
** Description      This function overwrite libnfc-tmsTransit.conf file
**                  with transitConfValue.
**
** Returns          bool.
**
*******************************************************************************/
bool phTmsNciHalSetTmsTransitConfig(char *transitConfValue);

/*******************************************************************************
**
** Function         phTmsNciHalEseSoftReset
**
** Description      It shall be used to reset eSE by proprietary command.
**
** Returns          status of eSE reset result
*******************************************************************************/
bool phTmsNciHalEseSoftReset();

/*******************************************************************************
**
** Function         phTmsNciHalNfccFwDownload
**
** Description      It shall be used to download nfcc fw.
**
** Returns          status of fw download result
*******************************************************************************/
bool phTmsNciHalNfccFwDownload();


/*******************************************************************************
**
** Function         doEseCosDownloadI2C
**
** Description      It shall be used to download ese cos by i2c.
**
** Returns          status of fw download result.
*******************************************************************************/
int16_t doEseCosDownloadI2C();

/*******************************************************************************
**
** Function         doNfccFwDownload
**
** Description      It shall be used to download nfcc fw.
**
** Returns          status of fw download result.
*******************************************************************************/
int16_t doNfccFwDownload();

/*******************************************************************************
**
** Function         doNfccBlDownload
**
** Description      It shall be used to download nfcc BL.
**
** Returns          status of fw download result.
*******************************************************************************/
int16_t doNfccBlDownload();
