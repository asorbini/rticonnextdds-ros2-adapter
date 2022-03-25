/* Copyright 2022 Real-Time Innovations, Inc. (RTI)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef RTICONNEXTDDS_ROS2_ADAPTER__CPP_HPP
#define RTICONNEXTDDS_ROS2_ADAPTER__CPP_HPP

#include <string>

#include "dds/dds.hpp"
#include "rticonnextdds_ros2_adapter/rticonnextdds_ros2_adapter_c.h"
#include "rticonnextdds_ros2_adapter/rticonnextdds_ros2_adapter_model.hpp"
#include "rticonnextdds_ros2_adapter/rticonnextdds_ros2_adapter_modelPlugin.hpp"

namespace rti {

namespace ros2 {

RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
extern const char * const GRAPH_TOPIC_NAME;

RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
extern const char * const GRAPH_TYPE_NAME;

typedef RTIROS2_GraphNodeHandle GraphNodeHandle;
typedef RTIROS2_GraphEndpointHandle GraphEndpointHandle;

RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
extern const GraphNodeHandle GraphNodeHandle_INVALID;

RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
extern const GraphEndpointHandle GraphEndpointHandle_INVALID;

enum class GraphEndpointType
{
  Unknown = RTIROS2_GRAPH_ENDPOINT_UNKNOWN,
  Subscription = RTIROS2_GRAPH_ENDPOINT_SUBSCRIPTION,
  Publisher = RTIROS2_GRAPH_ENDPOINT_PUBLISHER,
  Client = RTIROS2_GRAPH_ENDPOINT_CLIENT,
  Service = RTIROS2_GRAPH_ENDPOINT_SERVICE
};

struct GraphProperties
{
  dds::domain::DomainParticipant graph_participant{nullptr};
  dds::pub::Publisher graph_publisher{nullptr};
  dds::topic::Topic<RTIROS2::ParticipantEntitiesInfo> graph_topic{nullptr};
  dds::pub::DataWriter<RTIROS2::ParticipantEntitiesInfo> graph_writer{nullptr};
  DDS_Duration_t poll_period = {0, 0};
};

namespace graph {
  RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
  void
  customize_datawriter_qos(
    dds::pub::qos::DataWriterQos & writer_qos);

  RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
  void
  customize_datareader_qos(
    dds::sub::qos::DataReaderQos & reader_qos);

  RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
  void
  compute_participant_gid(
    dds::domain::DomainParticipant & dds_participant,
    RTIROS2::Gid & gid);

  RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
  void
  compute_reader_gid(
    dds::sub::AnyDataReader & dds_reader,
    RTIROS2::Gid & gid);

  RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
  void
  compute_writer_gid(
    dds::pub::AnyDataWriter & dds_writer,
    RTIROS2::Gid & gid);
  
  RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
  void
  compute_writer_topic_names(
    const std::string & ros2_topic_name,
    const std::string & ros2_type_name,
    const GraphEndpointType ros2_endp_type,
    std::string & dds_topic_name,
    std::string & dds_type_name);

  RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
  void
  compute_reader_topic_names(
    const std::string & ros2_topic_name,
    const std::string & ros2_type_name,
    const GraphEndpointType ros2_endp_type,
    std::string & dds_topic_name,
    std::string & dds_type_name);

  RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
  void
  compute_service_topic_names(
    const std::string & ros2_node_name,
    const std::string & ros2_service_name,
    const std::string & ros2_type_name,
    std::string & dds_req_topic_name,
    std::string & dds_req_type_name,
    std::string & dds_rep_topic_name,
    std::string & dds_rep_type_name);
}  // namespace graph

class Graph {
public:

  RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
  explicit Graph(const GraphProperties & properties);

  RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
  virtual ~Graph();

  RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
  dds::domain::DomainParticipant
  graph_participant() const;

  RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
  dds::pub::Publisher
  graph_publisher() const;

  RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
  dds::topic::Topic<RTIROS2::ParticipantEntitiesInfo>
  graph_topic() const;

  RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
  dds::pub::DataWriter<RTIROS2::ParticipantEntitiesInfo>
  graph_writer() const;

  RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
  GraphNodeHandle
  register_local_node(
    const std::string & node_name,
    const std::string & node_namespace = std::string(),
    dds::domain::DomainParticipant dds_participant = nullptr);

  RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
  void
  inspect_local_node(
    const GraphNodeHandle node_handle);

  RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
  GraphEndpointHandle
  register_local_subscription(
    const GraphNodeHandle node_handle,
    dds::sub::AnyDataReader sub_reader);

  RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
  GraphEndpointHandle
  register_local_publisher(
    const GraphNodeHandle node_handle,
    dds::pub::AnyDataWriter pub_writer);

  RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
  GraphEndpointHandle
  register_local_client(
    const GraphNodeHandle node_handle,
    dds::sub::AnyDataReader client_reader,
    dds::pub::AnyDataWriter client_writer);

  RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
  GraphEndpointHandle
  register_local_service(
    const GraphNodeHandle node_handle,
    dds::sub::AnyDataReader service_reader,
    dds::pub::AnyDataWriter service_writer);

  RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
  void
  unregister_local_node(const GraphNodeHandle node_handle);

  RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
  void
  unregister_local_subscription(
    const RTIROS2_GraphNodeHandle node_handle,
    dds::sub::AnyDataReader sub_reader);

  RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
  void
  unregister_local_publisher(
    const GraphNodeHandle node_handle,
    dds::pub::AnyDataWriter pub_writer);

  RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
  void
  unregister_local_client(
    const GraphNodeHandle node_handle,
    dds::sub::AnyDataReader client_reader,
    dds::pub::AnyDataWriter client_writer);

  RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
  void
  unregister_local_service(
    const GraphNodeHandle node_handle,
    dds::sub::AnyDataReader service_reader,
    dds::pub::AnyDataWriter service_writer);

  RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
  void
  unregister_local_subscription(
    const GraphNodeHandle node_handle,
    const GraphEndpointHandle sub_handle);

  RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
  void
  unregister_local_publisher(
    const GraphNodeHandle node_handle,
    const GraphEndpointHandle pub_handle);

  RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
  void
  unregister_local_client(
    const GraphNodeHandle node_handle,
    const GraphEndpointHandle client_handle);

  RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
  void
  unregister_local_service_by_handle(
    const GraphNodeHandle node_handle,
    const GraphEndpointHandle service_handle);

  RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
  dds::domain::DomainParticipant
  get_node_participant(const GraphNodeHandle node_handle);

  RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
  void
  get_node_name(
    const GraphNodeHandle node_handle,
    const char ** const name_out,
    const char ** const namespace_out);

  RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
  dds::core::optional<dds::sub::AnyDataReader>
  get_endpoint_reader(
    const GraphNodeHandle node_handle,
    const GraphEndpointHandle endp_handle);

  RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
  dds::core::optional<dds::pub::AnyDataWriter>
  get_endpoint_writer(
    const GraphNodeHandle node_handle,
    const GraphEndpointHandle endp_handle);

  RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
  GraphEndpointType
  get_endpoint_type(
    const GraphNodeHandle node_handle,
    const GraphEndpointHandle endp_handle);

  RTIROS2_ParticipantEntitiesInfo *
  c_sample()
  {
    return c_pinfo_;
  }

protected:
  dds::domain::DomainParticipant graph_participant_{nullptr};
  dds::pub::Publisher graph_publisher_{nullptr};
  dds::topic::Topic<RTIROS2::ParticipantEntitiesInfo> graph_topic_{nullptr};
  dds::pub::DataWriter<RTIROS2::ParticipantEntitiesInfo> graph_writer_{nullptr};
  DDS_Duration_t poll_period_ = {0, 0};
  RTIROS2_Graph * c_graph_{nullptr};
  RTIROS2_ParticipantEntitiesInfo * c_pinfo_{nullptr};
};

}  // namespace ros2
}  // namespace rti

#endif  /* RTICONNEXTDDS_ROS2_ADAPTER__CPP_HPP */