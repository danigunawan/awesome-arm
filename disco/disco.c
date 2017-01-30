#define TASKSTACKSIZE   768
/*
 *  ======== display.c ========
 */
/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/runtime/System.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>

/* TI-RTOS Header files */
#include <ti/drivers/PIN.h>
#include <ti/mw/display/Display.h>
#include <ti/mw/display/DisplayExt.h>

/* Board Header files */
#include "Board.h"


#define TASKSTACKSIZE   768

Task_Struct task0Struct;
Char task0Stack[TASKSTACKSIZE];

/* Pin driver handle */
static PIN_Handle ledPinHandle;
// For Led1
static PIN_Handle ledPinHandle1;
static PIN_State ledPinState;
// For Led2
static PIN_State ledPinState1;

/*
 * Application LED pin configuration table:
 *   - All LEDs board LEDs are off.
 */
PIN_Config ledPinTable[] = {
    Board_LED0 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    PIN_TERMINATE
};
 PIN_Config ledPinTable1[] = {
       Board_LED1 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
     PIN_TERMINATE
 };

/*
 *  ======== taskFxn ========
 *  Toggle the Board_LED0. The Task_sleep is determined by arg0 which
 *  is configured for the heartBeat Task instance.
 */
Void taskFxn(UArg arg0, UArg arg1)
{
    unsigned int ledPinValue;
    unsigned int ledPinValue1;

    /* Initialize display and try to open both UART and LCD types of display. */
    Display_Params params;
    Display_Params_init(&params);
    params.lineClearMode = DISPLAY_CLEAR_BOTH;

    /* Open both an available LCD display and an UART display.
     * Whether the open call is successful depends on what is present in the
     * Display_config[] array of the board file.
     *
     * Note that for SensorTag evaluation boards combined with the SHARP96x96
     * Watch DevPack, there is a pin conflict with UART such that one must be
     * excluded, and UART is preferred by default. To display on the Watch
     * DevPack, add the precompiler define BOARD_DISPLAY_EXCLUDE_UART.
     */
    Display_Handle hDisplayLcd = Display_open(Display_Type_LCD, &params);
    Display_Handle hDisplaySerial = Display_open(Display_Type_UART, &params);

    /* Check if the selected Display type was found and successfully opened */
    if (hDisplaySerial) {
        Display_print0(hDisplaySerial, 0, 0, "Hello Serial!");
    }

    /* Check if the selected Display type was found and successfully opened */
    if (hDisplayLcd) {
        Display_print0(hDisplayLcd, 5, 3, "Hello LCD!");

        /* Wait a while so text can be viewed. */
        Task_sleep(1000 * (1000/Clock_tickPeriod));

        /*
         * Use the GrLib extension to get the GraphicsLib context object of the
         * LCD, if it is supported by the display type.
         */
        tContext *pContext = DisplayExt_getGrlibContext(hDisplayLcd);

        /* It's possible that no compatible display is available. */
        if (pContext) {
            /* Draw splash */
            GrImageDraw(pContext, &splashImage, 0, 0);
            GrFlush(pContext);
        }
        else {
            /* Not all displays have a GraphicsLib back-end */
            Display_print0(hDisplayLcd, 0, 0, "Display driver");
            Display_print0(hDisplayLcd, 1, 0, "is not");
            Display_print0(hDisplayLcd, 2, 0, "GrLib capable!");
        }

        /* Wait for a bit, then clear */
        Task_sleep(5000 * (1000/Clock_tickPeriod));
        Display_clear(hDisplayLcd);
    }

    /* Loop forever, alternating LED state and Display output. */
    while (1) {
        ledPinValue = PIN_getOutputValue(Board_LED0);
        ledPinValue1 = PIN_getOutputValue(Board_LED1);

        if (hDisplayLcd) {
            /* Print to LCD and clear alternate lines if the LED is on or not. */
            Display_clearLine(hDisplayLcd, ledPinValue ? 0:1);
            Display_print1(hDisplayLcd, ledPinValue ? 1:0, 0, "LED: %s",
                (!ledPinValue) ? "On!":"Off!");
        }

        if (hDisplaySerial) {
            /* Print to UART */
            Display_print1(hDisplaySerial, 0, 0, "LED: %s",
                (!ledPinValue)?"On!":"Off!");
        }

        /* Toggle LED */
        PIN_setOutputValue(ledPinHandle, Board_LED0,
                           !PIN_getOutputValue(Board_LED0));
     PIN_setOutputValue(ledPinHandle1, Board_LED1,
                           !PIN_getOutputValue(Board_LED1));

        Task_sleep((UInt)arg0);
    }
}

/*
 *  ======== main ========
 */
int main(void)
{
    Task_Params taskParams;

    /* Call board init functions */
    Board_initGeneral();

    /* Construct heartBeat Task  thread */
    Task_Params_init(&taskParams);
    taskParams.arg0 = 1000000 / Clock_tickPeriod;
    taskParams.stackSize = TASKSTACKSIZE;
    taskParams.stack = &task0Stack;
    Task_construct(&task0Struct, (Task_FuncPtr)taskFxn, &taskParams, NULL);


    /* Open LED pins */
    ledPinHandle = PIN_open(&ledPinState, ledPinTable);
    if(!ledPinHandle) {
        System_abort("Error initializing board LED pins\n");
    }
    /* Open LED1 pins */
    // ledPinHandle1 = PIN_open(&ledPinState1, ledPinTable);
    // if(!ledPinHandle1) {
    //     System_abort("Error initializing board LED pins\n");
    // }

    PIN_setOutputValue(ledPinHandle, Board_LED0, 1);
//    Clock.tickPeriod = 10;
    PIN_setOutputValue(ledPinHandle1, Board_LED1, 0);

    System_printf("Starting the example\nSystem provider is set to SysMin. "
                  "Halt the target to view any SysMin contents in ROV.\n");

    /* SysMin will only print to the console when you call flush or exit */
    System_flush();

    /* Start BIOS */
    BIOS_start();

    return (0);
}
