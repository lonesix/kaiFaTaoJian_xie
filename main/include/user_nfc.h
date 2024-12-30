#ifndef _USER_NFC_H_
#define _USER_NFC_H_

/*********************************************************************
 * INCLUDES
 */
#include <stdint.h>
/*********************************************************************
 * DEFINITIONS
 */
#define NFC_COOLDOWN_PERIOD         5000      // 5s
#define NFC_EVENT                   1

/*********************************************************************
 * API FUNCTIONS
 */
void NFC_Init(void);
void CreateNfcCooldownTimer(void);
void StartNfcCooldownTimer(void);
void GetCardId(uint8_t *pData);
uint8_t GetNfcTriggerEvent(void);
void SetNfcTriggerEvent(uint8_t event);

#endif /* _USER_NFC_H_ */