#ifndef VESCLIB_H_
#define VESCLIB_H_

#include "Arduino.h"
#include "crc.h"
#include "datatypes.h"
#include "buffer.h"

// SetSerialPort sets the serial to communicate with the VESC
// Multiple ports possible
void SetSerialPort(HardwareSerial* _serialPort1, HardwareSerial* _serialPort2, HardwareSerial* _serialPort3, HardwareSerial* _serialPort4);
void SetSerialPort(HardwareSerial* _serialPort1, HardwareSerial* _serialPort2, HardwareSerial* _serialPort3);
void SetSerialPort(HardwareSerial* _serialPort1, HardwareSerial* _serialPort2);
void SetSerialPort(HardwareSerial* _serialPort1);

static HardwareSerial* serialPort1;
static HardwareSerial* serialPort2;
static HardwareSerial* serialPort3;
static HardwareSerial* serialPort4;
static usb_serial_class* debugSerialPort = NULL;

void SetSerialPort(HardwareSerial* _serialPort1, HardwareSerial* _serialPort2, HardwareSerial* _serialPort3, HardwareSerial* _serialPort4) {
	serialPort1 = _serialPort1;
	serialPort2 = _serialPort2;
	serialPort3 = _serialPort3;
	serialPort4 = _serialPort4;
	
}

void SetSerialPort(HardwareSerial* _serialPort1) {
	SetSerialPort(_serialPort1, _serialPort1, _serialPort1, _serialPort1);
}

void SetSerialPort(HardwareSerial* _serialPort1, HardwareSerial* _serialPort2) {
	SetSerialPort(_serialPort1, _serialPort2, _serialPort1, _serialPort1);
}

void SetSerialPort(HardwareSerial* _serialPort1,HardwareSerial* _serialPort2,HardwareSerial* _serialPort3) {
	SetSerialPort(_serialPort1, _serialPort2, _serialPort3, _serialPort1);
}

#include "ringbuffer.h"

enum possible_msg_states {
  OUT,
  START,
  TYPE,
  SIZE,
};

uint8_t msg_state[3] = {OUT, OUT, OUT};

int PackSendPayload(uint8_t* payload, int lenPay, int num) {
	uint16_t crcPayload = crc16(payload, lenPay);
	int count = 0;
	uint8_t messageSend[256];

	if (lenPay <= 256)
	{
		messageSend[count++] = 2;
		messageSend[count++] = lenPay;
	}
	else
	{
		messageSend[count++] = 3;
		messageSend[count++] = (uint8_t)(lenPay >> 8);
		messageSend[count++] = (uint8_t)(lenPay & 0xFF);
	}
	memcpy(&messageSend[count], payload, lenPay);

	count += lenPay;
	messageSend[count++] = (uint8_t)(crcPayload >> 8);
	messageSend[count++] = (uint8_t)(crcPayload & 0xFF);
	messageSend[count++] = 3;
	messageSend[count] = NULL;

	HardwareSerial *serial;

	switch (num) {
		case 0:
			serial=serialPort1;
			break;
		case 1:
			serial=serialPort2;
			break;
		case 2:
			serial=serialPort3;
			break;
		case 3:
			serial=serialPort4;
			break;
		default:
			break;
	}

	//Sending package
	serial->write(messageSend, count);


	//Returns number of send bytes
	return count;
}

void vesc_send_status_request(uint8_t serial_port) {
	uint8_t command[1] = { COMM_GET_VALUES };
	PackSendPayload(command, 1, serial_port);
}

void make_serial_available(uint8_t serial_port) {
	shift_rx_stream_to_buffer(serial_port);			// read from serial port into ringbuffer.
}

uint8_t find_status_message(uint8_t serial_port) 
{
	// Message Layout: total length = 64
	// 1: Start-Byte = 2
	// 2: Payload_Length-uint8 = 59 for a COMM_GET_VALUES
	// 3: Type-uint8 = 4 for a COMM_GET_VALUES
	// 4...x: payload bytes
	// x+1: crc0-Byte
	// x+2: crc1-Byte
	// x+3: End-Byte = 3

	/* This is how VESC generates the message
  	------------------------------------------------------------------------------
	ind = 0;
	send_buffer[ind++] = COMM_GET_VALUES;
	buffer_append_float16(send_buffer, mc_interface_temp_fet_filtered(), 1e1, &ind);
	buffer_append_float16(send_buffer, mc_interface_temp_motor_filtered(), 1e1, &ind);
	buffer_append_float32(send_buffer, mc_interface_read_reset_avg_motor_current(), 1e2, &ind);
	buffer_append_float32(send_buffer, mc_interface_read_reset_avg_input_current(), 1e2, &ind);
	buffer_append_float32(send_buffer, mc_interface_read_reset_avg_id(), 1e2, &ind);
	buffer_append_float32(send_buffer, mc_interface_read_reset_avg_iq(), 1e2, &ind);
	buffer_append_float16(send_buffer, mc_interface_get_duty_cycle_now(), 1e3, &ind);
	buffer_append_float32(send_buffer, mc_interface_get_rpm(), 1e0, &ind);
	buffer_append_float16(send_buffer, GET_INPUT_VOLTAGE(), 1e1, &ind);
	buffer_append_float32(send_buffer, mc_interface_get_amp_hours(false), 1e4, &ind);
	buffer_append_float32(send_buffer, mc_interface_get_amp_hours_charged(false), 1e4, &ind);
	buffer_append_float32(send_buffer, mc_interface_get_watt_hours(false), 1e4, &ind);
	buffer_append_float32(send_buffer, mc_interface_get_watt_hours_charged(false), 1e4, &ind);
	buffer_append_int32(send_buffer, mc_interface_get_tachometer_value(false), &ind);
	buffer_append_int32(send_buffer, mc_interface_get_tachometer_abs_value(false), &ind);
	send_buffer[ind++] = mc_interface_get_fault();
	buffer_append_float32(send_buffer, mc_interface_get_pid_pos_now(), 1e6, &ind);
	send_buffer[ind++] = app_get_configuration()->controller_id;
	commands_send_packet(send_buffer, ind=59);
  	------------------------------------------------------------------------------
	and this is how the message is send with handler_num=0:
  	------------------------------------------------------------------------------
	void packet_send_packet(unsigned char *data, unsigned int len, int handler_num) {
		int b_ind = 0;
		if (len <= 256) {
			handler_states[handler_num].tx_buffer[b_ind++] = 2;
			handler_states[handler_num].tx_buffer[b_ind++] = len;
		} else {
			handler_states[handler_num].tx_buffer[b_ind++] = 3;
			handler_states[handler_num].tx_buffer[b_ind++] = len >> 8;
			handler_states[handler_num].tx_buffer[b_ind++] = len & 0xFF;
		}

		memcpy(handler_states[handler_num].tx_buffer + b_ind, data, len);
		b_ind += len;

		unsigned short crc = crc16(data, len);
		handler_states[handler_num].tx_buffer[b_ind++] = (uint8_t)(crc >> 8);
		handler_states[handler_num].tx_buffer[b_ind++] = (uint8_t)(crc & 0xFF);
		handler_states[handler_num].tx_buffer[b_ind++] = 3;

		if (handler_states[handler_num].send_func) {
			handler_states[handler_num].send_func(handler_states[handler_num].tx_buffer, b_ind);
		}
	}
	------------------------------------------------------------------------------
	*/

  	/* // implementation 2
	static uint8_t msg_len = 0;
	static uint8_t msg_type = 0;
	const uint8_t max_payload = 59;

	while (Serial_available(serial_port)) {
		if (msg_state[serial_port] == OUT) 							// curser before Start-Byte
		{ // waiting for start byte '2'
			if (Serial_read(serial_port) == 2) {
				msg_state[serial_port] = START;
			} else {
				msg_state[serial_port] = OUT;
			}
		}
		else if (msg_state[serial_port] == START) 			// curser before Length-Byte
		{
			msg_len = Serial_peek(serial_port, 0);
			if (msg_len <= max_payload) {
				msg_state[serial_port] = SIZE;
			} else {
				msg_state[serial_port] = OUT;
			}
		}
		else if (msg_state[serial_port] == SIZE) 				// curser before Length-Byte
		{
			msg_type = Serial_peek(serial_port, 1);
			switch (msg_type) {
				case COMM_GET_VALUES:
					msg_state[serial_port] = TYPE;
					break;
				default:
				  msg_state[serial_port] = OUT;
					break;
			}
		}
		else if (msg_state[serial_port] == TYPE) 				// curser before Length-Byte
		{
			// wait for entire message to arrive
			if (Serial_available(serial_port) < (1 + msg_len + 2 + 1)) {
				break;
			}
			else
			{
				// check if End-Byte is valid
				if (Serial_peek(serial_port, 1 + msg_len + 2) != 3) {
					msg_state[serial_port] = OUT;
					break;
				}
				// check if CRC is valid
				uint16_t crc_comp = (((uint16_t)Serial_peek(serial_port, 1 + msg_len) << 8) & 0xFF00) + ((uint16_t)Serial_peek(serial_port, 1 + msg_len + 1) & 0xFF);
				if (crc_comp != calculate_crc_from_ring_buffer(serial_port, 1, msg_len)) {
					msg_state[serial_port] = OUT;
					break;
				}
				// message is ready
				Serial_flush(serial_port, 1);							// curser now before Type-Byte
				msg_state[serial_port] = OUT;
				return msg_len;
				// NOTE: message must be read from the ringbuffer immediatly!
			}
		}
	}
	return false;
  */ // end implementation 2

  // implementation 1
	static uint8_t msg_len = 0;
	while (Serial_available(serial_port)) 
	{
		Serial.print("serial available ");
		Serial.print(Serial_available(serial_port));
		Serial.print("  msg_state ");
		Serial.println(msg_state[serial_port]);
		if (msg_state[serial_port] == OUT) 							// curser before Start-Byte
		{ // waiting for start byte '2'
			msg_state[serial_port] = (Serial_read(serial_port) == 2) ? START : OUT;
		}
		else if (msg_state[serial_port] == START && Serial_available(serial_port) >= 2)  // curser before Payload_Length-Byte
		{ // identify type and discard message if type is not COMM_GET_VALUES
			msg_state[serial_port] = (Serial_peek(serial_port, 1) == COMM_GET_VALUES) ? TYPE : OUT;
		}
		else if (msg_state[serial_port] == TYPE) 				// curser before Payload_Length-Byte
		{ // message type is validated, now get length
			msg_len = Serial_peek(serial_port);
			msg_state[serial_port] = SIZE;
			
		}
		else if ((msg_state[serial_port] == SIZE) && (Serial_available(serial_port) >= (1 + msg_len + 2))) // curser before Payload_Length-Byte
		{
			
			Serial.print("  msg_state[serial_port] == SIZE ");
			for (uint8_t i=0 ; i<=msg_len+1+2+1; i++) {
				Serial.print((uint8_t)Serial_peek(serial_port, i));
				Serial.print(" ");
			}
			Serial.println("end");
			// check if End-Byte is valid ; curser is before Payload_Length-Byte
			/*
			if (Serial_peek(serial_port, 1 + msg_len + 2) != 3) 
			{
				msg_state[serial_port] = OUT;
				continue;
			}
			*/

			Serial.print("  endbyte okay ");
			// validate via crc ; curser is before Payload_Length-Byte
			uint16_t crc_comp = (((uint16_t)Serial_peek(serial_port, 1 + msg_len) << 8) & 0xFF00) + ((uint16_t)Serial_peek(serial_port,1 + msg_len + 1) & 0xFF);
			if (crc_comp == calculate_crc_from_ring_buffer(serial_port, 1, msg_len)) 
			{
				Serial_flush(serial_port, 1); 					// move curser before Type-Byte, delete length byte
				msg_state[serial_port] = OUT;
				
				Serial.print("message received ---------- ");
				Serial.println(msg_len);
				return msg_len;
			}
			else 
			{
				Serial.print("  crc not okay ");
				msg_state[serial_port] = OUT;
				continue;
			}
		}
		else break;
	}
	return false;
}

bool vesc_compute_receive(uint8_t serial_port) {
    make_serial_available(serial_port);
    find_status_message(serial_port);
    return true;
}

uint8_t VescUartGetValue(bldcMeasure& values, uint8_t serial_port) {
	static byte message_content[70];
  	make_serial_available(serial_port);
	uint8_t msg_len = find_status_message(serial_port);
	uint8_t msg_type = 0;
	Serial.print("got length ");
	Serial.println(msg_len);
	if (msg_len) {
		Serial.print("len");
		// read message type from Type-Byte
		msg_type = Serial_read(serial_port);		// curser before payload
		Serial.println(msg_type);
		// read message from ringbuffer into a byte array
		transfer_data_from_ring_to_array(serial_port, message_content, msg_len - 1); // curser now before crc-bytes
		setRGBled(255,255,0);

		// flush crc and end-bytes
		Serial_flush(serial_port, 2 + 1);		// curser now behind end of message-container

		// get the pointer to the first byte of this array
		uint8_t *message = message_content;
		int32_t ind = 0;

		values.tempFetFiltered		= buffer_get_float16(message, 1e1, &ind);			// buffer_append_float16(send_buffer, mc_interface_temp_fet_filtered(), 1e1, &ind);
		values.tempMotorFiltered	= buffer_get_float16(message, 1e1, &ind);			// buffer_append_float16(send_buffer, mc_interface_temp_motor_filtered(), 1e1, &ind);
		values.avgMotorCurrent		= buffer_get_float32(message, 100.0, &ind);		// buffer_append_float32(send_buffer, mc_interface_read_reset_avg_motor_current(), 1e2, &ind);
		values.avgInputCurrent		= buffer_get_float32(message, 100.0, &ind);		// buffer_append_float32(send_buffer, mc_interface_read_reset_avg_input_current(), 1e2, &ind);
		values.avgId				= buffer_get_float32(message, 1e2, &ind);						// buffer_append_float32(send_buffer, mc_interface_read_reset_avg_id(), 1e2, &ind);
		values.avgIq				= buffer_get_float32(message, 1e2, &ind);						// buffer_append_float32(send_buffer, mc_interface_read_reset_avg_iq(), 1e2, &ind);
		values.dutyNow				= buffer_get_float16(message, 1000.0, &ind);			// buffer_append_float16(send_buffer, mc_interface_get_duty_cycle_now(), 1e3, &ind);
		values.rpm					= buffer_get_float32(message, 1.0, &ind);						// buffer_append_float32(send_buffer, mc_interface_get_rpm(), 1e0, &ind);
		values.inpVoltage			= buffer_get_float16(message, 10.0, &ind);				// buffer_append_float16(send_buffer, GET_INPUT_VOLTAGE(), 1e1, &ind);
		values.ampHours				= buffer_get_float32(message, 10000.0, &ind);			// buffer_append_float32(send_buffer, mc_interface_get_amp_hours(false), 1e4, &ind);
		values.ampHoursCharged		= buffer_get_float32(message, 10000.0, &ind);	// buffer_append_float32(send_buffer, mc_interface_get_amp_hours_charged(false), 1e4, &ind);
		values.wattHours			= buffer_get_float32(message, 1e4, &ind);					// buffer_append_float32(send_buffer, mc_interface_get_watt_hours(false), 1e4, &ind);
		values.watthoursCharged		= buffer_get_float32(message, 1e4, &ind);			// buffer_append_float32(send_buffer, mc_interface_get_watt_hours_charged(false), 1e4, &ind);
		values.tachometer			= buffer_get_int32(message, &ind);								// buffer_append_int32(send_buffer, mc_interface_get_tachometer_value(false), &ind);
		values.tachometerAbs		= buffer_get_int32(message, &ind);							// buffer_append_int32(send_buffer, mc_interface_get_tachometer_abs_value(false), &ind);
		values.faultCode			= message[ind];																		// send_buffer[ind++] = mc_interface_get_fault();
																																						// buffer_append_float32(send_buffer, mc_interface_get_pid_pos_now(), 1e6, &ind);
																																						// send_buffer[ind++] = app_get_configuration()->controller_id;
	}
	return msg_type;
}

void VescUartSetCurrent(float current, int num) {
	int32_t index = 0;
	uint8_t payload[5];

	payload[index++] = COMM_SET_CURRENT ;
	buffer_append_int32(payload, (int32_t)(current * 1000), &index);
	PackSendPayload(payload, 5, num);
}

void VescUartSetCurrentBrake(float brakeCurrent, int num) {
	int32_t index = 0;
	uint8_t payload[5];

	payload[index++] = COMM_SET_CURRENT_BRAKE;
	buffer_append_int32(payload, (int32_t)(brakeCurrent * 1000), &index);
	PackSendPayload(payload, 5, num);
}

void VescUartSetRPM(float rpm, int num) {
	int32_t index = 0;
	uint8_t payload[5];

	payload[index++] = COMM_SET_RPM ;
	buffer_append_int32(payload, (int32_t)(rpm), &index);
	PackSendPayload(payload, 5, num);
}

void VescUartSetDuty(float duty, int num) {
	int32_t index = 0;
	uint8_t payload[5];

	payload[index++] = COMM_SET_DUTY ;
	buffer_append_int32(payload, (int32_t)(duty * 100000), &index);
	PackSendPayload(payload, 5, num);
}

void VescUartSetPosition(float position, int num) {
	int32_t index = 0;
	uint8_t payload[5];

	payload[index++] = COMM_SET_POS ;
	buffer_append_int32(payload, (int32_t)(position * 1000000.0), &index);
	PackSendPayload(payload, 5, num);
}

#endif // VESCLIB_H_