/*----------------------------------------------------------------------------
LAB EXERCISE - Simple BLE Appcessory
 ----------------------------------------
	In this exercise we will create a BLE Heart Rate Monitor sensor which transmits dummy data to
  other devices.

	GOOD LUCK!
 *----------------------------------------------------------------------------*/

#include "mbed.h"
#include "BLEDevice.h"
#include "HeartRateService.h"
#include "BatteryService.h"
#include "DeviceInformationService.h"
#include "Utils.h" /* To turn on/off the debug messages on the console edit this file; NEED_CONSOLE_OUTPUT 1/0( it will have an impact on code-size and power consumption.) */

BLEDevice ble;
DigitalOut led(LED1);
Ticker update;
Ticker aliveness;

const static char DEVICE_NAME[] = "MY_BLE_HRM";
static const uint16_t uuid16_list[] = {GattService::UUID_HEART_RATE_SERVICE,
                                       GattService::UUID_BATTERY_SERVICE,
                                       GattService::UUID_DEVICE_INFORMATION_SERVICE};

static volatile bool triggerSensorPolling = false;

void disconnectionCallback(Gap::Handle_t handle, Gap::DisconnectionReason_t reason)
{
    DEBUG("Disconnected!\n\r");
    DEBUG("Restarting the advertising process\n\r");
    ble.startAdvertising(); // restart advertising
}

void connectionTimeoutCallback()
{
    DEBUG("Connection Timeout\r\n");
    ble.startAdvertising();
}

void connectionCallback(Gap::Handle_t handle, const Gap::ConnectionParams_t *reason)
{
    DEBUG("Connected\r\n");
}

void update_handler(void)
{
    /* Note that the periodicCallback() executes in interrupt context, so it is safer to do
  *  heavy-weight sensor polling from the main thread. */
    triggerSensorPolling = true;
}

void aliveness_handler(void)
{
    led = !led;
}

/*----------------------------------------------------------------------------
 MAIN function
 *----------------------------------------------------------------------------*/

int main(void)
{
    update.attach(update_handler, 1);
    aliveness.attach(aliveness_handler, 0.5);

    DEBUG("Initialising \n\r");
    ble.init();
    DEBUG("\r\nAdding Device Name");
    ble.setDeviceName((uint8_t*)DEVICE_NAME);
    DEBUG("\r\n\r\n");
    ble.onDisconnection(disconnectionCallback);
    ble.onConnection(connectionCallback);
    ble.onTimeout(connectionTimeoutCallback);

    /* Setup primary service. */
    uint8_t hrmCounter = 100;
    HeartRateService hrService(ble, hrmCounter, HeartRateService::LOCATION_FINGER);

    /* Setup auxiliary services. */
    uint8_t batterylevel = 25;
    BatteryService battery(ble, batterylevel);
    DeviceInformationService deviceInfo(ble, "ST", "Nucleo", "SN1");

    /* Setup advertising. */
    ble_error_t myWatchDog = BLE_ERROR_NONE;

    myWatchDog = ble.accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED | GapAdvertisingData::LE_GENERAL_DISCOVERABLE);
    if( myWatchDog != BLE_ERROR_NONE ) { DEBUG("??? acc payload\r\n"); myWatchDog = BLE_ERROR_NONE; }
    myWatchDog = ble.accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LIST_16BIT_SERVICE_IDS, (uint8_t *)uuid16_list, sizeof(uuid16_list));
    if( myWatchDog != BLE_ERROR_NONE ) { DEBUG("??? acc payload\r\n"); myWatchDog = BLE_ERROR_NONE; }
    myWatchDog = ble.accumulateAdvertisingPayload(GapAdvertisingData::GENERIC_HEART_RATE_SENSOR);
    if( myWatchDog != BLE_ERROR_NONE ) { DEBUG("??? acc payload\r\n"); myWatchDog = BLE_ERROR_NONE; }
    ble.accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LOCAL_NAME, (uint8_t *)DEVICE_NAME, sizeof(DEVICE_NAME));
    if( myWatchDog != BLE_ERROR_NONE ) { DEBUG("??? acc payload\r\n"); myWatchDog = BLE_ERROR_NONE; }

    ble.setAdvertisingType(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED);
    ble.setAdvertisingInterval(1600); /* 1000ms; in multiples of 0.625ms. */

    myWatchDog = ble.startAdvertising();
    if (myWatchDog != BLE_ERROR_NONE)
    {
        DEBUG("this is in theory possible; error code:%d\n\r", myWatchDog);
    }
    else
    {
        DEBUG("the board shall be broken.\n\r");
    }

    while (true)
    {
        if (triggerSensorPolling & ble.getGapState().connected)
        {
            triggerSensorPolling = false;

            /* Do blocking calls or whatever is necessary for sensor polling. */
            /* In our case, we simply update the dummy HRM measurement. */
            hrmCounter++;

            if (hrmCounter == 150)
            {
                hrmCounter = 80;
            }

            hrService.updateHeartRate(hrmCounter);

            DEBUG("now hr: %d", hrmCounter);
        }
        else
        {
            static int my_counter = 0;
            my_counter++;
            wait(1.5);
            Gap::GapState_t my_gapstate;
            my_gapstate = ble.getGapState();
            DEBUG("\r\nconnected:   %d\r\n", my_gapstate.connected);
            DEBUG("advertising: %d\r\n", my_gapstate.advertising);

            ble.waitForEvent();
        }
    }
}

// *******************************ARM University Program Copyright � ARM Ltd 2014*************************************//
