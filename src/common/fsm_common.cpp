//
// Created by peter on 2020/12/3.
//

#include "rm_fsm/common/fsm_common.h"

template<typename T>
State<T>::State(ros::NodeHandle &fsm_nh, FsmData<T> *fsm_data, std::string state_name)
    : fsm_nh_(fsm_nh), data_(fsm_data), state_name_(std::move(state_name)) {

}

template<typename T>
bool State<T>::loadParam() {
  if (!fsm_nh_.getParam("control_param/accel_x", accel_x_) ||
      !fsm_nh_.getParam("control_param/accel_y", accel_y_) ||
      !fsm_nh_.getParam("control_param/accel_angular", accel_angular_) ||
      !fsm_nh_.getParam("control_param/brake_multiple", brake_multiple_) ||
      !fsm_nh_.getParam("control_param/expect_shoot_hz", expect_shoot_hz_) ||
      !fsm_nh_.getParam("control_param/safe_shoot_hz", safe_shoot_hz_) ||
      !fsm_nh_.getParam("control_param/safe_shoot_speed", safe_shoot_speed_) ||
      !fsm_nh_.getParam("control_param/gimbal_error_limit", gimbal_error_limit_) ||
      !fsm_nh_.getParam("power_limit/safety_power", safety_power_) ||
      !fsm_nh_.getParam("power_limit/have_power_manager", have_power_manager_)) {
    ROS_ERROR("Some fsm params doesn't given (namespace: %s)", fsm_nh_.getNamespace().c_str());
    return false;
  }

  if (control_mode_ == "pc") { // pc mode
    if (!fsm_nh_.getParam("control_param/pc_param/coefficient_x", coefficient_x_) ||
        !fsm_nh_.getParam("control_param/pc_param/coefficient_y", coefficient_y_) ||
        !fsm_nh_.getParam("control_param/pc_param/coefficient_angular", coefficient_angular_) ||
        !fsm_nh_.getParam("control_param/pc_param/coefficient_yaw", coefficient_yaw_) ||
        !fsm_nh_.getParam("control_param/pc_param/coefficient_pitch", coefficient_pitch_)) {
      ROS_ERROR("Some fsm params doesn't given (namespace: %s)", fsm_nh_.getNamespace().c_str());
      return false;
    }
  } else if (control_mode_ == "rc") { // rc mode
    if (!fsm_nh_.getParam("control_param/rc_param/coefficient_x", coefficient_x_) ||
        !fsm_nh_.getParam("control_param/rc_param/coefficient_y", coefficient_y_) ||
        !fsm_nh_.getParam("control_param/rc_param/coefficient_angular", coefficient_angular_) ||
        !fsm_nh_.getParam("control_param/rc_param/coefficient_yaw", coefficient_yaw_) ||
        !fsm_nh_.getParam("control_param/rc_param/coefficient_pitch", coefficient_pitch_)) {
      ROS_ERROR("Some fsm params doesn't given (namespace: %s)", fsm_nh_.getNamespace().c_str());
      return false;
    }
  } else {
    ROS_ERROR("Cannot find responding control mode (pc & rc)");
    return false;
  }

  return true;
}

template<typename T>
uint8_t State<T>::getShootSpeedCmd(int shoot_speed) {
  switch (shoot_speed) {
    case 10: return rm_msgs::ShootCmd::SPEED_10M_PER_SECOND;
    case 15: return rm_msgs::ShootCmd::SPEED_15M_PER_SECOND;
    case 16: return rm_msgs::ShootCmd::SPEED_16M_PER_SECOND;
    case 18: return rm_msgs::ShootCmd::SPEED_18M_PER_SECOND;
    case 30: return rm_msgs::ShootCmd::SPEED_30M_PER_SECOND;
    default: return 0;
  }
}

template<typename T>
void State<T>::setChassis(uint8_t chassis_mode,
                          double linear_x,
                          double linear_y,
                          double angular_z,
                          const ros::Time &now) {
  double accel_x = accel_x_;
  double accel_y = accel_y_;
  double accel_angular = accel_angular_;

  data_->chassis_cmd_.mode = chassis_mode;

  if (linear_x == 0.0)
    accel_x = accel_x_ * brake_multiple_;

  if (linear_y == 0.0)
    accel_y = accel_y_ * brake_multiple_;

  if (angular_z == 0.0)
    accel_angular = accel_angular_ * brake_multiple_;
  data_->chassis_cmd_.accel.linear.x = accel_x;
  data_->chassis_cmd_.accel.linear.y = accel_y;
  data_->chassis_cmd_.accel.angular.z = accel_angular;
  //power limit
  if (have_power_manager_) {//have power manger
    data_->chassis_cmd_.power_limit = data_->referee_->power_manager_data_.parameters[1];
  } else if (!(have_power_manager_)
      && data_->referee_->referee_data_.game_robot_status_.max_HP != 0) {//do not have power manger and use referee data
    data_->chassis_cmd_.power_limit = data_->referee_->referee_data_.game_robot_status_.chassis_power_limit;
    if (data_->chassis_cmd_.power_limit > 120) data_->chassis_cmd_.power_limit = 120;
  } else {//use safety power
    data_->chassis_cmd_.power_limit = safety_power_;
  }
  data_->chassis_cmd_.stamp = now;

  data_->cmd_vel_.linear.x = linear_x * coefficient_x_;
  data_->cmd_vel_.linear.y = linear_y * coefficient_y_;
  data_->cmd_vel_.angular.z = angular_z * coefficient_angular_;

  data_->vel_cmd_pub_.publish(data_->cmd_vel_);
  data_->chassis_cmd_pub_.publish(data_->chassis_cmd_);
}

template<typename T>
void State<T>::setGimbal(uint8_t gimbal_mode, double rate_yaw, double rate_pitch,
                         uint8_t target_id, double bullet_speed, const ros::Time &now) {
  data_->gimbal_cmd_.mode = gimbal_mode;

  data_->gimbal_cmd_.rate_yaw = rate_yaw * coefficient_yaw_;
  data_->gimbal_cmd_.rate_pitch = rate_pitch * coefficient_pitch_;
  data_->gimbal_cmd_.stamp = now;

  data_->gimbal_cmd_.target_id = target_id;
  data_->gimbal_cmd_.bullet_speed = bullet_speed;
  data_->gimbal_cmd_pub_.publish(data_->gimbal_cmd_);
}

template<typename T>
void State<T>::setShoot(uint8_t shoot_mode, int shoot_speed, double shoot_hz, const ros::Time &now) {
  data_->shoot_cmd_.mode = shoot_mode;

  data_->shoot_cmd_.speed = getShootSpeedCmd(shoot_speed);
  data_->shoot_cmd_.hz = shoot_hz;
  data_->shoot_cmd_.stamp = now;

  data_->shooter_cmd_pub_.publish(data_->shoot_cmd_);
}

template<typename T>
void State<T>::setControlMode(const std::string &control_mode) {
  control_mode_ = control_mode;
}

/**
 * Constructor for the Control FSM. Passes in all of the necessary
 * data and stores it in a struct. Initializes the FSM with a starting
 * state and operating mode.
 * @tparam T
 * @param nh
 */
template<typename T>
Fsm<T>::Fsm(ros::NodeHandle &node_handle):fsm_nh_(node_handle) {
  tf_listener_ = new tf2_ros::TransformListener(tf_);

  data_.init(fsm_nh_);

  string2state.insert(std::make_pair("invalid", nullptr));
  // Initialize a new FSM State with the control data
  current_state_ = string2state["invalid"];

  // Initialize to not be in transition
  next_state_ = current_state_;

  // Initialize FSM mode to normal operation
  operating_mode_ = FsmOperatingMode::kNormal;
}

/**
 * Called each control loop iteration. Decides if the robot is safe to
 * run controls and checks the current state for any transitions. Runs
 * the regular state behavior if all is normal.
 */
template<typename T>
void Fsm<T>::run() {
  // TODO: Safety check

  // run referee system
  data_.referee_->read();
  // Run the robot control code if operating mode is not unsafe
  if (operating_mode_ != FsmOperatingMode::kEStop) {
    // Run normal controls if no transition is detected
    if (operating_mode_ == FsmOperatingMode::kNormal) {
      // Check the current state for any transition
      next_state_name_ = getDesiredState();

      // Detect a commanded transition
      if (next_state_name_ != current_state_->state_name_) {
        // Set the FSM operating mode to transitioning
        operating_mode_ = FsmOperatingMode::kTransitioning;

        // Get the next FSM State by name
        next_state_ = string2state[next_state_name_];
      } else {
        // Update control mode (pc/rc)
        current_state_->setControlMode(control_mode_);
        // Run the iteration for the current state normally
        current_state_->run();
      }
    }

    // Run the transition code while transition is occuring
    if (operating_mode_ == FsmOperatingMode::kTransitioning) {
      // Check the robot state for safe operation
      // TODO: Safety post check.

      // Exit the current state cleanly
      current_state_->onExit();
      // Complete the transition
      current_state_ = next_state_;
      // Enter the new current state cleanly
      current_state_->onEnter();
      // Update control mode (pc/rc)
      current_state_->setControlMode(control_mode_);

      // Return the FSM to normal operation mode
      operating_mode_ = FsmOperatingMode::kNormal;
    } else {
      // Check the robot state for safe operation
      // TODO: Safety post check.
    }
  } else { // if ESTOP
    current_state_ = string2state["passive"];
    ROS_INFO("Current state is passive.");
    next_state_name_ = current_state_->state_name_;
  }
  // draw UI
  if (data_.referee_->is_open_) {
    data_.referee_->run();
  }
}

// RobotRunner a template
template
class Fsm<float>;
template
class Fsm<double>;

template
class State<float>;
template
class State<double>;