#ifndef WAVEMAP_ROS_WAVEMAP_SERVER_H_
#define WAVEMAP_ROS_WAVEMAP_SERVER_H_

#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <glog/logging.h>
#include <ros/ros.h>
#include <wavemap/common.h>
#include <wavemap/config/config_base.h>
#include <wavemap/data_structure/volumetric/volumetric_data_structure_base.h>
#include <wavemap/utils/thread_pool.h>

#include "wavemap_ros/inputs/input_base.h"
#include "wavemap_ros/operations/operation_base.h"
#include "wavemap_ros/utils/logging_level.h"
#include "wavemap_ros/utils/tf_transformer.h"

namespace wavemap {
/**
 * Config struct for wavemap's ROS server.
 */
struct WavemapServerConfig : ConfigBase<WavemapServerConfig, 4, LoggingLevel> {
  //! Name of the coordinate frame in which to store the map.
  //! Will be used as the frame_id for ROS TF lookups.
  std::string world_frame = "odom";
  //! Minimum severity level for ROS logging messages to be logged.
  LoggingLevel logging_level = LoggingLevel::kInfo;
  //! Maximum number of threads to use.
  //! Defaults to the number of threads supported by the CPU.
  int num_threads =
      std::max(1, static_cast<int>(std::thread::hardware_concurrency()));
  //! Whether or not to allow resetting the map through the reset_map service.
  bool allow_reset_map_service = false;

  static MemberMap memberMap;

  bool isValid(bool verbose) const override;
};

class WavemapServer {
 public:
  WavemapServer(ros::NodeHandle nh, ros::NodeHandle nh_private);
  WavemapServer(ros::NodeHandle nh, ros::NodeHandle nh_private,
                const WavemapServerConfig& config);

  InputBase* addInput(const param::Value& integrator_params,
                      const ros::NodeHandle& nh, ros::NodeHandle nh_private);
  OperationBase* addOperation(const param::Value& operation_params,
                              ros::NodeHandle nh_private);

  void runOperations(const ros::Time& current_time, bool force_run_all = false);

  VolumetricDataStructureBase::Ptr getMap() { return occupancy_map_; }
  VolumetricDataStructureBase::ConstPtr getMap() const {
    return occupancy_map_;
  }

  bool saveMap(const std::filesystem::path& file_path) const;
  bool loadMap(const std::filesystem::path& file_path);

 private:
  const WavemapServerConfig config_;

  // Map data structure
  VolumetricDataStructureBase::Ptr occupancy_map_;

  // Threadpool shared among all input handlers and operations
  std::shared_ptr<ThreadPool> thread_pool_;

  // Transform and depth inputs
  std::shared_ptr<TfTransformer> transformer_;
  std::vector<std::unique_ptr<InputBase>> input_handlers_;

  // Operations to perform after map updates
  std::vector<std::unique_ptr<OperationBase>> operations_;

  // ROS services
  void advertiseServices(ros::NodeHandle& nh_private);
  ros::ServiceServer reset_map_srv_;
  ros::ServiceServer save_map_srv_;
  ros::ServiceServer load_map_srv_;
};
}  // namespace wavemap

#endif  // WAVEMAP_ROS_WAVEMAP_SERVER_H_
