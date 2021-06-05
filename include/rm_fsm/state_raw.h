//
// Created by astro on 2020/12/8.
//

#ifndef RM_FSM_STATE_RAW_H
#define RM_FSM_STATE_RAW_H
#include "rm_fsm/common/fsm_common.h"

namespace rm_fsm {
class StateRaw : public StateBase {
 public:
  StateRaw(ros::NodeHandle &nh, Data *data, const std::string &state_string)
      : StateBase(nh, data, state_string) {}
 protected:
  void setChassis() override {
    chassis_cmd_sender_->setMode(rm_msgs::ChassisCmd::RAW);
    vel_2d_cmd_sender_->setLinearXVel(data_->dbus_data_.ch_r_y);
  }
  void setGimbal() override {
    if (data_->dbus_data_.s_l == rm_msgs::DbusData::DOWN) {
      gimbal_cmd_sender_->setMode(rm_msgs::GimbalCmd::RATE);
      gimbal_cmd_sender_->setRate(-data_->dbus_data_.ch_l_x, -data_->dbus_data_.ch_l_y);
    } else gimbal_cmd_sender_->setMode(rm_msgs::GimbalCmd::TRACK);
  }
  void setShooter() override {
    if (data_->dbus_data_.s_l == rm_msgs::DbusData::UP) shooter_cmd_sender_->setMode(rm_msgs::ShootCmd::PUSH);
    else if (data_->dbus_data_.s_l == rm_msgs::DbusData::MID) shooter_cmd_sender_->setMode(rm_msgs::ShootCmd::READY);
    else shooter_cmd_sender_->setMode(rm_msgs::ShootCmd::STOP);
  }
};
}

#endif //RM_FSM_STATE_RAW_H
