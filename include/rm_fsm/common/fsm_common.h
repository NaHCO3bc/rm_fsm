//
// Created by peter on 2020/12/3.
//

#ifndef SRC_RM_SOFTWARE_RM_DECISION_SRC_FSM_FSM_STATE_H_
#define SRC_RM_SOFTWARE_RM_DECISION_SRC_FSM_FSM_STATE_H_

#include <iostream>
#include <queue>
#include <utility>
#include <tf/transform_listener.h>
#include <control_toolbox/pid.h>
#include <rm_common/ros_utilities.h>
#include "rm_common/ori_tool.h"
#include "fsm_data.h"

/**
 * A base fsm state class for all robots.
 * @tparam T
 */
template<typename T>
class State {
 public:
  // Generic constructor fo all states
  State(ros::NodeHandle &fsm_nh, FsmData<T> *fsm_data, std::string state_string);

  ros::NodeHandle fsm_nh_;

  // Behavior to be carried out when entering a state
  virtual void onEnter() = 0;

  // Run the normal behavior for the state
  virtual void run() = 0;

  // Behavior to be carried out when exiting a state
  virtual void onExit() = 0;

  // Load params from yaml file
  bool loadParam();

  uint8_t getShootSpeedCmd(int shoot_speed);

  // Base controllers.
  void setChassis(uint8_t chassis_mode, double linear_x, double linear_y, double angular_z, const ros::Time &now);
  void setGimbal(uint8_t gimbal_mode, double rate_yaw, double rate_pitch,
                 uint8_t target_id, double bullet_speed, const ros::Time &now);
  void setShoot(uint8_t shoot_mode, int shoot_speed, double shoot_hz, const ros::Time &now);

  void setControlMode(const std::string &control_mode);

  // Holds all of the relevant control data
  FsmData<T> *data_;

  // FSM State info
  std::string state_name_;     // enumerated name of the current state

  tf2_ros::Buffer tf_;
  tf2_ros::TransformListener *tf_listener_;

  std::string control_mode_;
  uint8_t graph_operate_type_;

  // chassis fsm control accelerate
  double accel_x_ = 0.0;
  double accel_y_ = 0.0;
  double accel_angular_ = 0.0;
  int brake_multiple_ = 1;

  // chassis fsm control coefficient
  double coefficient_x_ = 0.0;
  double coefficient_y_ = 0.0;
  double coefficient_angular_ = 0.0;

  // gimbal fsm control coefficient
  double coefficient_yaw_ = 0.0;
  double coefficient_pitch_ = 0.0;
  double gimbal_error_limit_ = 2.0;

  double expect_shoot_hz_ = 0.0;
  double safe_shoot_hz_ = 0.0;
  double safe_shoot_speed_ = 0;
  double actual_shoot_speed_ = 0;
  int ultimate_shoot_speed_ = 0;

  double safety_power_ = 0;
  bool have_power_manager_ = false;

  uint8_t last_chassis_mode_;
  uint8_t last_shoot_mode_;
  double last_angular_z_;
};

/**
 * Enumerate all of the operating modes
 */
enum class FsmOperatingMode {
  kNormal, kTransitioning, kEStop, kEDamp
};

/**
 * Control FSM handles the FSM states from a higher level
 */
template<typename T>
class Fsm {
 public:
  explicit Fsm(ros::NodeHandle &nh);

  ros::NodeHandle fsm_nh_;

  //Pointer list of each state.
  std::map<std::string, State<T> *> string2state;

  // Runs the FSM logic and handles the state transitions and normal runs
  void run();
  // Get desired state decided by control fsm data.
  virtual std::string getDesiredState() = 0;

  // Send related data to FsmState
  FsmData<T> data_;

  State<T> *current_state_;    // current FSM state
  State<T> *next_state_;       // next FSM state
  std::string next_state_name_;  // next FSM state name

  tf2_ros::Buffer tf_;
  tf2_ros::TransformListener *tf_listener_;

  std::string control_mode_; // pc control or rc control

 private:
  // Operating mode of the FSM
  FsmOperatingMode operating_mode_{};
};

#endif //SRC_RM_SOFTWARE_RM_DECISION_SRC_FSM_FSM_STATE_H_