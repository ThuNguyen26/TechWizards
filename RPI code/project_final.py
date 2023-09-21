from bluepy import btle
import json
import time
import datetime
from paho.mqtt import client as mqttclient
temp = 0
humidiy = 0
CO_ppm = 0
MAC = "60:a4:23:c9:83:48"
broker_address="103.1.238.175"
port=1885
user="test"
password="testadmin"
topic = [("FPT/led_control/led1", 0), ("FPT/led_control/led2", 0), ("FPT/led_control/led3", 0), ("FPT/led_control/led4", 0), ("FPT/led_control/led5", 0)]
topic_data = "FPT/data"
SERVICE_UUID_Control = "89393252-d387-4209-87a9-fe25915f4a1f"
CHAR_UUID_LED1 = "6ddcb5d5-8d2e-47f9-bfd4-05d705a03851"
CHAR_UUID_LED2 = "1b86c9ff-161b-4fbc-be27-efaa4dfda0c1"
CHAR_UUID_LED3 = "df0508b5-721f-41d4-971b-d577778fffd1"
CHAR_UUID_LED4 = "1b17b736-2702-4873-93f3-bf30dade1bcd"
CHAR_UUID_LED5 = "f3770495-9053-4fe1-9dfa-2838c28a5fbd"
a = bytes(chr(1), 'utf-8')
b = bytes(chr(0), 'utf-8')

def connect_mqtt():
	def on_connect(client, userdata, flags, rc):
		if rc == 0:
			print("Connected to MQTT Broker!")
		else:
			print("Failed to connect, return code %d\n", rc)

	client= mqttclient.Client("PI")
	client.username_pw_set(user,password=password)
	client.connect(broker_address,port=port)
	client.on_connect=on_connect
	client.subscribe(topic)
	return client
client = connect_mqtt()

class MyDelegate(btle.DefaultDelegate):
	def __init__(self):
		btle.DefaultDelegate.__init__(self)

	def handleNotification(self, cHandle, data):
		global temp
		global humidity
		global CO_ppm
		if cHandle == 27:
			temp = ((data[2] & 0x7F) << 8 | data[3]) / 10.00
			humidity = ((data[0] << 8) | data[1]) / 10.00
			CO_ppm = (data[5] << 8 | data[6]) / 1000.000
		if cHandle == 30:
			voltage = (data[0] << 8 | data[1]) / 10.00
			current = (data[2] << 8 | data[3] | data[4] << 24 | data[5] << 16) / 1000.00
			power = (data[6] << 8 | data[7] | data[8] << 24 | data[9] << 16) / 10.00
			energy = (data[10] << 8 | data[11] | data[12] << 24 | data[13] << 16) / 1000.000
			freq = (data[14] << 8 | data[15]) / 10.0
			pf = (data[16] << 8 | data[17]) / 100.00
			now = datetime.datetime.now()
			timestamp = int(now.timestamp())
			data_str = {
				"Device ID": MAC,
				"Timestamp": timestamp + 11,
				"Temperature": temp,
				"Humidity": humidity,
				"CO_ppm": CO_ppm,
				"Voltage": voltage,
				"Current": current,
				"Power": power,
				"Energy": energy,
				"Frequency": freq,
				"Power Factor": pf
			}
			json_data = json.dumps(data_str)
			print("---------------------------")
			print(json_data)
			result = client.publish(topic_data, json_data)
			status = result[0]
			if(status == 0):
				print("Success")
			else:
				print("Failed")

p = btle.Peripheral(MAC)
p.setDelegate( MyDelegate() )

# Setup to turn notifications on, e.g.
svc = p.getServiceByUUID("d7a08db4-18ba-45b5-8af6-b0bf5888d2c3")
ch = svc.getCharacteristics("add09c57-3b31-4f82-b770-4dd125f96473")[0]
ch1 = svc.getCharacteristics("f0b60e4a-8e0d-4c54-b752-1169e0294bcd")[0]
p.writeCharacteristic(ch.valHandle+1, b"\x01\x00", withResponse = True)
p.writeCharacteristic(ch1.valHandle+1, b"\x01\x00", withResponse = True)
svc_control = p.getServiceByUUID("89393252-d387-4209-87a9-fe25915f4a1f")
ch_led1 = svc_control.getCharacteristics(CHAR_UUID_LED1)[0]
ch_led2 = svc_control.getCharacteristics(CHAR_UUID_LED2)[0]
ch_led3 = svc_control.getCharacteristics(CHAR_UUID_LED3)[0]
ch_led4 = svc_control.getCharacteristics(CHAR_UUID_LED4)[0]
ch_led5 = svc_control.getCharacteristics(CHAR_UUID_LED5)[0]
def control(topic_RX, topicTX, message, ch):
        if topic_RX == topicTX:
                if message == "ON":
                        ch.write(a, withResponse=True)
                elif message == "OFF":
                        ch.write(b, withResponse=True)
def on_message(client,userdata,message):
	topic_rec = str(message.topic)
	message_rec = str(message.payload.decode("utf-8"))
	print(topic_rec + ": " + message_rec)
	control(topic_rec, "FPT/led_control/led1", message_rec, ch_led1)
	control(topic_rec, "FPT/led_control/led2", message_rec, ch_led2)
	control(topic_rec, "FPT/led_control/led3", message_rec, ch_led3)
	control(topic_rec, "FPT/led_control/led4", message_rec, ch_led4)
	control(topic_rec, "FPT/led_control/led5", message_rec, ch_led5)
client.on_message = on_message
#client.loop_forever()
while True:
	client.loop_start()
	if p.waitForNotifications(5.0):
		continue
	client.loop_stop()
