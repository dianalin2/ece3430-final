#include <ti/devices/msp/msp.h>
#include "../../LP-MSPM0G3507/bsp.h"
#include "../../LP-MSPM0G3507/uart.h"
#include "../../LP-MSPM0G3507/clock.h"
#include "../../LP-MSPM0G3507/MKII.h"
#include "../../LP-MSPM0G3507/LCD.h"
#include "../../LP-MSPM0G3507/ADC.h"
#include "../../LP-MSPM0G3507/mspm0g350x_int.h"
#include "FSM.h"
#include <stdio.h>
#include <stdlib.h>

// Defines
#define FALSE 0
#define TRUE 1

#define BUFFER_LENGTH 64
#define FEEDBACK_LENGTH 33

#define JOYSTICK_X_PORT GPIOA
#define JOYSTICK_X_PIN 25

#define JOYSTICK_Y_PORT GPIOA
#define JOYSTICK_Y_PIN 18

#define FULL_FORWARD_MAGNITUDE 0.75


// Boolean structure to determine if pushbutton has been pushed.
/*typedef struct {
    uint8_t LeftWheelsSpeed;
    uint8_t RightWheelsSpeed;
} MotorSpeeds;*/


// Function Definitions
uint8_t ConvertToCurrent(float speed);
FSMState NextStateTable(FSMType *FSM);
void OutputLCDMenu(FSMType *FSM);
void Group1CallbackFunction(void *CallbackObject);
void UpdateWriteControl(ControlPacket *LaunchpadPacket);
void UpdateReadFeedback();


// Global variable to configure UART
UART_Config UART0_Config =
{
 .Tx_index = PA10INDEX,
 .Tx_iomuxMode = IOMUX_MODE2,
 .Rx_index = PA11INDEX,
 .Rx_iomuxMode = IOMUX_MODE2,
 .ClockFrequency = 0
};


// Global variables
Callback_s Group1 = {(void *) 0,Group1CallbackFunction};  // VERIFY IF PA12 OR BPS2 IS IN GROUP 1
uint32_t ClockFrequency = 32000000;
uint8_t FeedbackReceiveBuffer[1 + FEEDBACK_LENGTH];
FeedbackPacket RobotPacket;
ControlPacket LaunchpadPacket;

int main(void)
{
    // Declare UI Menu FSM
    FSMType UI_Menu_FSM =
    {.CurrentInputS2 = INACTIVE,
     .CurrentState = UI_Image,
     .NextStateFuncPtr = NextStateTable,
     .OutputFuncPtr = OutputLCDMenu};

    // Declare priority level, pushbutton int, and callback object as address of pushbutton int
    uint32_t priority = 2; // interrupt priority
    uint32_t PB2 = FALSE; // pushbutton 2
    Group1.CallbackObject = (void *) &PB2; // Update Group 1 callback object pointer.


    InitializeBoosterpack(ClockFrequency);
    UART0_Config.ClockFrequency = ClockFrequency;
    InitializeUART(UART0,&UART0_Config);

    // ADC Initialization
    ADC0_Init(2,ADCVREF_VDDA); // PA25 ADC0 channel 2 J1.2 MKII Joystick X
    ADC1_Init(3,ADCVREF_VDDA); // PA18 ADC1 channel 3 J3.26 MKII Joystick Y


    // Enable interrupt for button 2
     __disable_irq();
    BP_S2_PORT->POLARITY15_0 = BP_S2_POL_RISING;        // Rising-edge interrupt (when button is released)
    BP_S2_PORT->CPU_INT.ICLR =  BP_S2_MASK;            // Clear interrupt bit
    BP_S2_PORT->CPU_INT.IMASK = BP_S2_MASK;           // Enable the S2 interrupt.

    // Enable NVIC interrupt, set interrupt priority, and enable interrupt.
    __NVIC_EnableIRQ(BP_S2_INTERRUPT);
    __NVIC_SetPriority(BP_S2_INTERRUPT,priority);
    __enable_irq();

    // LCD Init
    LCD_Init();
    LCD_FillScreen(LCD_BLACK);
    LCD_SetTextColor(LCD_CYAN);

    // ControlPacket Init
    LaunchpadPacket = (ControlPacket){.startByte = 0xFF, .headerByte = 0x00, .leftMotorSpeed = 127, .rightMotorSpeed = 127};

    while(TRUE){
        UI_Menu_FSM.CurrentInputS2 = PB2;

        if(PB2){
            PB2 = FALSE;
        }

        if(UI_Menu_FSM.CurrentState != UI_Menu_FSM.NextStateFuncPtr(&UI_Menu_FSM)){
            LCD_FillScreen(LCD_BLACK);
        }

        UI_Menu_FSM.CurrentState = UI_Menu_FSM.NextStateFuncPtr(&UI_Menu_FSM);
        UI_Menu_FSM.OutputFuncPtr(&UI_Menu_FSM);

        UpdateReadFeedback();
    }
    
    return 0;
}          

/* MOTOR SPEED CONVERSION RELATED */
uint8_t ConvertToCurrent(float speed){
    float scaled_speed = (speed + 1.0f) * 127.0f;
    if(scaled_speed < 0){
        scaled_speed = 0;
    } 
    else if(scaled_speed > 254){
        scaled_speed = 254;
    }
    return (uint8_t)scaled_speed;
}


void UpdateWriteControl(ControlPacket *LaunchpadPacket){
    float joystickXAxis = (ADC0_In() / 2047.5f) - 1;
    float joystickYAxis = (ADC1_In() / 2047.5f) - 1;
    float linearComponent = joystickYAxis * FULL_FORWARD_MAGNITUDE;
    float angularComponent = joystickXAxis * (FULL_FORWARD_MAGNITUDE);
    uint8_t LeftSpeed = ConvertToCurrent(linearComponent + angularComponent);
    uint8_t RightSpeed = ConvertToCurrent(linearComponent - angularComponent);

    LaunchpadPacket->leftMotorSpeed = LeftSpeed;
    LaunchpadPacket->rightMotorSpeed = RightSpeed;
    UART_WriteControl(LaunchpadPacket, sizeof(LaunchpadPacket));
}

void UpdateReadFeedback(){
    if((UART0->STAT&0x04) == 0x04)
        return;


    for (int i = 0; i < FEEDBACK_LENGTH + 1; i++) {
        FeedbackReceiveBuffer[i] = GetUserInput(UART0);
        if (i % 30 == 0)
            UpdateWriteControl(&LaunchpadPacket);
    }

    memcpy(&(RobotPacket.distance), &FeedbackReceiveBuffer[6], sizeof(float));
    memcpy(&(RobotPacket.x_accel), &FeedbackReceiveBuffer[10], sizeof(float));
    memcpy(&(RobotPacket.y_accel), &FeedbackReceiveBuffer[14], sizeof(float));
    memcpy(&(RobotPacket.z_accel), &FeedbackReceiveBuffer[18], sizeof(float));
    memcpy(&(RobotPacket.yaw), &FeedbackReceiveBuffer[22], sizeof(float));
    memcpy(&(RobotPacket.pitch), &FeedbackReceiveBuffer[26], sizeof(float));
    memcpy(&(RobotPacket.roll), &FeedbackReceiveBuffer[30], sizeof(float));
}


/* LCD FSM RELATED */
const FSMState TransitionTable[2][2] =
{
 // 0 and 1
 {UI_Image, UI_Feedback},
 {UI_Feedback, UI_Image}
};

FSMState NextStateTable(FSMType *FSM)
{
    FSMState NextState = FSM->CurrentState;

    uint32_t TableRow = (uint32_t)(FSM->CurrentState);
    uint32_t TableCol = (FSM->CurrentInputS2);

    NextState = TransitionTable[TableRow][TableCol];
    return NextState;
}


void OutputLCDMenu(FSMType *FSM)
{
    uint8_t OutputBuffer[BUFFER_LENGTH];
    switch (FSM->CurrentState){ 
        case UI_Image:
            snprintf(OutputBuffer,12,"Driver View");
            LCD_DrawString(5,0,OutputBuffer,LCD_RED);

            snprintf(OutputBuffer, sizeof(OutputBuffer), "L: %u\n", LaunchpadPacket.leftMotorSpeed);
            LCD_DrawString(1,2,OutputBuffer,LCD_CYAN);

            snprintf(OutputBuffer, sizeof(OutputBuffer), "R: %u\n", LaunchpadPacket.leftMotorSpeed);
            LCD_DrawString(13,2,OutputBuffer,LCD_CYAN);

            LCD_FillRect(34,40,60,80,LCD_WHITE);

            LCD_FillRect(22,40,12,25,(LCD_WHITE - (LCD_CYAN * fabs((LaunchpadPacket.leftMotorSpeed - 121) / 225.0f))));
            LCD_FillRect(94,40,12,25,(LCD_WHITE - (LCD_CYAN * fabs((LaunchpadPacket.rightMotorSpeed - 121) / 225.0f))));
            LCD_FillRect(22,95,12,25,(LCD_WHITE - (LCD_CYAN * fabs((LaunchpadPacket.leftMotorSpeed - 121) / 225.0f))));
            LCD_FillRect(94,95,12,25,(LCD_WHITE - (LCD_CYAN * fabs((LaunchpadPacket.rightMotorSpeed - 121) / 225.0f))));
            break;
        case UI_Feedback:
            snprintf(OutputBuffer,9,"Feedback");
            LCD_DrawString(7,0,OutputBuffer,LCD_RED);

            snprintf(OutputBuffer, sizeof(OutputBuffer), "Obj Distance: %f\n", RobotPacket.distance);
            LCD_DrawString(0,2,OutputBuffer,LCD_CYAN);

            snprintf(OutputBuffer, sizeof(OutputBuffer), "Accel X: %f\n", RobotPacket.x_accel);
            LCD_DrawString(0,3,OutputBuffer,LCD_CYAN);

            snprintf(OutputBuffer, sizeof(OutputBuffer), "Accel Y: %f\n", RobotPacket.y_accel);
            LCD_DrawString(0,4,OutputBuffer,LCD_CYAN);

            snprintf(OutputBuffer, sizeof(OutputBuffer), "Accel Z: %f\n", RobotPacket.z_accel);
            LCD_DrawString(0,5,OutputBuffer,LCD_CYAN);

            snprintf(OutputBuffer, sizeof(OutputBuffer), "Yaw: %f\n", RobotPacket.yaw);
            LCD_DrawString(0,6,OutputBuffer,LCD_CYAN);

            snprintf(OutputBuffer, sizeof(OutputBuffer), "Pitch: %f\n", RobotPacket.pitch);
            LCD_DrawString(0,7,OutputBuffer,LCD_CYAN);

            snprintf(OutputBuffer, sizeof(OutputBuffer), "Roll: %f\n", RobotPacket.roll);
            LCD_DrawString(0,8,OutputBuffer,LCD_CYAN);

            break;
    }
}

/* INTERRUPT FOR PUSHBUTTON LCD MENU */
void Group1CallbackFunction(void *CallbackObject)
{
    uint32_t *PB2 = (uint32_t*)CallbackObject;

    uint32_t S2_IIDXValue = BP_S2_PORT->CPU_INT.IIDX;

    if(S2_IIDXValue == (BP_S2_PIN+1)){
        *PB2 = TRUE;
    }
}