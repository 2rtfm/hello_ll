#include "keyled.h"

#define BUTTON_DELAY 20

#ifdef KEY_UP_Pin
int ScanKey(void) {
  if (LL_GPIO_IsInputPinSet(KEY_UP_GPIO_Port, KEY_UP_Pin)) {
    LL_mDelay(BUTTON_DELAY);
    while (LL_GPIO_IsInputPinSet(KEY_UP_GPIO_Port, KEY_UP_Pin))
      ;
    LL_mDelay(BUTTON_DELAY);
    return SET;
  } else {
    LL_mDelay(BUTTON_DELAY);
  }
  return RESET;
}
#endif
