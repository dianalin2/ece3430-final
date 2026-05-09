#ifndef FSM_H_
#define FSM_H_

#include <ti/devices/msp/msp.h>
#include "../../LP-MSPM0G3507/bsp.h"

// Defines for pushbutton values
#define INACTIVE 0
#define ACTIVE 1

// FSM state type
typedef enum {
    UI_Image, UI_Feedback
} FSMState;

typedef struct FSMStruct {
    uint32_t CurrentInputS2;                              // Current input of the FSM (S2)
    FSMState CurrentState;                                // Current state of the FSM
    FSMState (*NextStateFuncPtr)(struct FSMStruct *ptr);  // Next-state function pointer
    void (*OutputFuncPtr)(struct FSMStruct *ptr);         // Output function pointer
} FSMType;

typedef struct FSMPtrStruct {
    const FSMState CurrentState;
    const struct FSMPtrStruct *NextState[4];   // Next state of the FSM
} FSMPtrType;

#endif /* FSM_H_ */
