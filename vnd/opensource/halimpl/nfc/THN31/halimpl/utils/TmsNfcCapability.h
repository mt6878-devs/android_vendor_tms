/******************************************************************************
 *
 *  Copyright 2015-2018,2020-2021 NXP
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

/******************************************************************************
 *
 *  The original Work has been changed by Tsingteng MicroSystem.
 *
 *  Copyright (C) 2021-2022 Tsingteng MicroSystem
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  NOT A CONTRIBUTION
 ******************************************************************************/

#ifndef CAP_H
#define CAP_H
#include "TmsFeatures.h"
#define PCONFIGFL       (capability::getInstance())

class capability {
  private:
    static capability *instance;
    const uint16_t offsetFwRomCodeVersion = 4;
    const uint16_t offsetFwMinorVersion = 6;
    const uint16_t offsetFwMajorVersion = 7;
    /*product[] will be used to print product version and
    should be kept in accordance with NfcChipType*/
    const char *product[13] = {"UNKNOWN",
                               "thn31"
                              };
    capability();

  public:
    static NfcChipType chipType;
    static capability *getInstance();
    NfcChipType processChipType(uint8_t *pMsg, uint16_t msgLen);
    uint32_t getFWVersionInfo(uint8_t *pMsg, uint16_t msgLen);
};
#endif
