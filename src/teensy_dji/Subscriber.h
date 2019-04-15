#ifndef SUBSCRIBER_HPP
#define SUBSCRIBER_HPP

#include <uavcan/uavcan.hpp>
#include "phoenix_msgs/MotorTarget.hpp"
#include "phoenix_msgs/MotorConfig.hpp"
#include "Publisher.h"

using namespace uavcan;
using namespace phoenix_msgs;

// subscribing tasks:
// we want to subscribe the MotorTarget messages to set the motors.

Subscriber<MotorTarget> *motor_target_Subscriber;
Subscriber<MotorConfig> *mcconf_Subscriber;

void motor_target_callback(const MotorTarget &msg)
{
  v_veh();
  //double sign_v = sgn(vveh)
  if (check_arm_state())
  {
    if (sgn(vveh) == sgn(msg.current_rear_left))
    {
      VescUartSetCurrent(msg.current_rear_left, 0);
    }
    else
    {
      VescUartSetCurrentBrake(abs(msg.current_rear_left), 0);
    }
    if (sgn(vveh) == sgn(msg.current_rear_right))
    {
      VescUartSetCurrent(msg.current_rear_right, 1);
    }
    else
    {
      VescUartSetCurrentBrake(abs(msg.current_rear_right), 1);
    }

    // set servos
    steering_servo_position_3 = steering_servo_offset_3 + (float)msg.servo_rear_left;
    steering_servo_3.write(steering_servo_position_3);

    steering_servo_position_4 = steering_servo_offset_4 + (float)msg.servo_rear_right;
    steering_servo_4.write(steering_servo_position_4);
  }
  last_motor_target_receive = millis();
}

void initSubscriber(Node<NodeMemoryPoolSize> *node)
{
  // create a subscriber
  motor_target_Subscriber = new Subscriber<MotorTarget>(*node);
  mcconf_Subscriber = new Subscriber<MotorConfig>(*node);

  if (motor_target_Subscriber->start(motor_target_callback) < 0)
  {
    Serial.println("Unable to start motor_target_Subscriber!");
  }

  if(mcconf_Subscriber->start(motor_config_callback) < 0)
  {
    Serial.println("Unable to start motor_config_Subscriber!");
  }
}

#endif