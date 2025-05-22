#include "em_common.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "app_timer.h"
#include "sl_sleeptimer.h"
#include "app.h"
#include "gatt_db.h"
#include <stdlib.h>
#define DHT_PORT gpioPortC
#define DHT_PIN 7

DHT dht;
Pzem p;

//extern short sample;
// Periodic timer handle.
static app_timer_t app_periodic_timer_data, app_periodic_timer_control;
uint8_t data_recv;
size_t data_recv_len;
bool connection = false;
uint64_t last_connect = 0;
// Periodic timer callback.
static void app_periodic_timer_cb_data(app_timer_t *timer);
static void app_periodic_timer_cb_control(app_timer_t *timer);
// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;

SL_WEAK void app_init(void)
{
	init(&dht, DHT_PORT, DHT_PIN);
	Pzem_Init(&p, sl_iostream_mikroe_handle, 0xF8);
	CO_intit();
	GPIO_PinModeSet(gpioPortA, 4, gpioModePushPull, 0); //ledstatus
	GPIO_PinModeSet(gpioPortC, 3, gpioModePushPull, 0); //led1
	GPIO_PinModeSet(gpioPortA, 0, gpioModePushPull, 0); //led2
	GPIO_PinModeSet(gpioPortB, 3, gpioModePushPull, 0); //led3
	GPIO_PinModeSet(gpioPortB, 4, gpioModePushPull, 0); //led4
	GPIO_PinModeSet(gpioPortC, 6, gpioModePushPull, 0); //led5
}

SL_WEAK void app_process_action(void)
{
	if(connection){
			if(sl_sleeptimer_get_tick_count64() - last_connect > 66000){
					GPIO_PinOutClear(gpioPortA, 4);
					app_timer_stop(&app_periodic_timer_control);
			}
	}
}

static void app_periodic_timer_cb_data(app_timer_t *timer){
	float temp = readTemperature(&dht, false);
	float humidity = readHumidity(&dht, false);
	updateValues(&p);
	float voltage = p.voltage;
	float current = p.current;
	float power = p.power;
	float energy = p.energy;
	float freq = p.freq;
	float pf = p.pf;
	float CO_ppm = getPPM();
	short CO_data = (short)(CO_ppm * 1000);
	dht.data[5] = CO_data >> 8;
	dht.data[6] = CO_data & 0xFF;
	sl_bt_gatt_server_notify_all(gattdb_air_quality, 7, dht.data);
	sl_bt_gatt_server_write_attribute_value(gattdb_air_quality, 0, 7, dht.data);
	sl_bt_gatt_server_notify_all(gattdb_electric, 18, p.data);
	sl_bt_gatt_server_write_attribute_value(gattdb_electric, 0, 18, p.data);
	app_log("-----------------------------------\n");
	app_log_info("Temperature: %f *C\n", temp);
	app_log_info("Humidity: %f %c\n", humidity, 0x25);
	app_log_info("Voltage: %f V\n", voltage);
	app_log_info("Current: %f A\n", current);
	app_log_info("Power: %f W\n", power);
	app_log_info("Energy: %f kWh\n", energy);
	app_log_info("Frequency: %f Hz\n", freq);
	app_log_info("Power factor: %f\n", pf);
	app_log_info("CO ppm: %f ppm\n", CO_ppm);
}
static void app_periodic_timer_cb_control(app_timer_t *timer){
	GPIO_PinOutToggle(gpioPortA, 4);
}

void sl_bt_on_event(sl_bt_msg_t *evt)
{
  sl_status_t sc;
  bd_addr address;
  uint8_t address_type;
  switch (SL_BT_MSG_ID(evt->header)) {
    case sl_bt_evt_system_boot_id:
        // Extract unique ID from BT Address.
        sc = sl_bt_system_get_identity_address(&address, &address_type);
        app_assert_status(sc);

        app_log_info("Bluetooth %s address: %02X:%02X:%02X:%02X:%02X:%02X\n",
                     address_type ? "static random" : "public device",
                     address.addr[5],
                     address.addr[4],
                     address.addr[3],
                     address.addr[2],
                     address.addr[1],
                     address.addr[0]);
        sc = sl_bt_advertiser_create_set(&advertising_set_handle);
        app_assert_status(sc);
        sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                   sl_bt_advertiser_general_discoverable);
        app_assert_status(sc);
        sc = sl_bt_advertiser_set_timing(
          advertising_set_handle, // advertising set handle
          160, // min. adv. interval (milliseconds * 1.6)
          160, // max. adv. interval (milliseconds * 1.6)
          0,   // adv. duration
          0);  // max. num. adv. events
        app_assert_status(sc);
        sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                           sl_bt_advertiser_connectable_scannable);
        app_assert_status(sc);
        app_log_info("Started advertising\n");
        app_timer_start(&app_periodic_timer_control, 1000, app_periodic_timer_cb_control, NULL, true);
      break;

    case sl_bt_evt_connection_opened_id:
      app_log_info("Connection opened\n");
      app_timer_stop(&app_periodic_timer_control);
      connection = true;
      app_timer_start(&app_periodic_timer_control, 100, app_periodic_timer_cb_control, NULL, true);
      last_connect = sl_sleeptimer_get_tick_count64();
      app_timer_start(&app_periodic_timer_data, 3000, app_periodic_timer_cb_data, NULL, true);
      break;

    case sl_bt_evt_connection_closed_id:
      // Generate data for advertising
      sc = app_timer_stop(&app_periodic_timer_data);
      connection = false;
      app_timer_start(&app_periodic_timer_control, 1000, app_periodic_timer_cb_control, NULL, true);
      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                 sl_bt_advertiser_general_discoverable);
      app_assert_status(sc);
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_advertiser_connectable_scannable);
      app_assert_status(sc);
      app_log_info("Started advertising\n");
      break;

    case sl_bt_evt_gatt_server_attribute_value_id:
    	app_log("AAA\n");
    	sc = sl_bt_gatt_server_read_attribute_value(evt->data.evt_gatt_server_attribute_value.attribute,
    	  											  0,
    	  											  sizeof(data_recv),
    	  											  &data_recv_len,
    	  											  &data_recv);
    	(void)data_recv_len;
    	app_log_status_error(sc);
    	app_log_info("%d\n", data_recv);
    	if (sc != SL_STATUS_OK)
    	 {
    	  break;
    	 }
      	switch(evt->data.evt_gatt_server_attribute_value.attribute){
      		case gattdb_led1:
      			GPIO_PinModeSet(gpioPortC, 3, gpioModePushPull, data_recv);
      			break;
      		case gattdb_led2:
      			GPIO_PinModeSet(gpioPortA, 0, gpioModePushPull, data_recv);
      			break;
      		case gattdb_led3:
      			GPIO_PinModeSet(gpioPortB, 3, gpioModePushPull, data_recv);
      			break;
      		case gattdb_led4:
      			GPIO_PinModeSet(gpioPortB, 4, gpioModePushPull, data_recv);
      			break;
      		case gattdb_led5:
      			GPIO_PinModeSet(gpioPortC, 6, gpioModePushPull, data_recv);
      			break;
      	}
        break;
    default:
      break;
  }
}
