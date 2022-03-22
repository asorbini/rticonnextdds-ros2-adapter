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

#ifndef rti_ros2_graph_h
#define rti_ros2_graph_h

#include "ndds/ndds_c.h"

extern const char * const RTI_ROS2_GRAPH_TOPIC_NAME;

extern const char * const RTI_ROS2_GRAPH_TYPE_NAME;

extern const char * const RTI_ROS2_GRAPH_THREAD_NAME;

typedef struct RTI_Ros2GraphI RTI_Ros2Graph;
typedef DDS_Long RTI_Ros2GraphNodeHandle;
typedef DDS_Long RTI_Ros2GraphEndpointHandle;

#define RTI_Ros2GraphNodeHandle_INVALID (-1)
#define RTI_Ros2GraphEndpointHandle_INVALID (-1)

struct RTI_Ros2GraphProperties
{
  DDS_DomainParticipant * dds_participant;
  DDS_Publisher * dds_publisher;
  DDS_Topic * graph_topic;
  DDS_DataWriter * graph_writer;
};

#define RTI_Ros2GraphProperties_INITIALIZER \
{\
  NULL /* dds_participant */,\
  NULL /* dds_publisher */,\
  NULL /* graph_topic */,\
  NULL /* graph_writer */\
}

RTI_Ros2Graph *
RTI_Ros2Graph_new(
  const struct RTI_Ros2GraphProperties * const properties);

DDS_ReturnCode_t
RTI_Ros2Graph_initialize(
  RTI_Ros2Graph * const self,
  const struct RTI_Ros2GraphProperties * const properties);

void
RTI_Ros2Graph_delete(RTI_Ros2Graph * const self);

DDS_ReturnCode_t
RTI_Ros2Graph_finalize(RTI_Ros2Graph * const self);

RTI_Ros2GraphNodeHandle
RTI_Ros2Graph_register_local_node(
  RTI_Ros2Graph * const self,
  const char * const node_name,
  const char * const node_namespace);

RTI_Ros2GraphEndpointHandle
RTI_Ros2Graph_register_local_subscription(
  RTI_Ros2Graph * const self,
  const RTI_Ros2GraphNodeHandle node_handle,
  DDS_DataReader * const sub_reader);

RTI_Ros2GraphEndpointHandle
RTI_Ros2Graph_register_local_publisher(
  RTI_Ros2Graph * const self,
  const RTI_Ros2GraphNodeHandle node_handle,
  DDS_DataWriter * const pub_writer);

RTI_Ros2GraphEndpointHandle
RTI_Ros2Graph_register_local_client(
  RTI_Ros2Graph * const self,
  const RTI_Ros2GraphNodeHandle node_handle,
  DDS_DataWriter * const client_writer,
  DDS_DataReader * const client_reader);

RTI_Ros2GraphEndpointHandle
RTI_Ros2Graph_register_local_service(
  RTI_Ros2Graph * const self,
  const RTI_Ros2GraphNodeHandle node_handle,
  DDS_DataWriter * const service_writer,
  DDS_DataReader * const service_reader);

DDS_ReturnCode_t
RTI_Ros2Graph_unregister_local_node(
  RTI_Ros2Graph * const self,
  const RTI_Ros2GraphNodeHandle node_handle);

DDS_ReturnCode_t
RTI_Ros2Graph_unregister_local_subscription(
  RTI_Ros2Graph * const self,
  const RTI_Ros2GraphNodeHandle node_handle,
  DDS_DataReader * const sub_reader);

DDS_ReturnCode_t
RTI_Ros2Graph_unregister_local_publisher(
  RTI_Ros2Graph * const self,
  const RTI_Ros2GraphNodeHandle node_handle,
  DDS_DataWriter * const pub_writer);

DDS_ReturnCode_t
RTI_Ros2Graph_unregister_local_client(
  RTI_Ros2Graph * const self,
  const RTI_Ros2GraphNodeHandle node_handle,
  DDS_DataReader * const client_reader,
  DDS_DataWriter * const client_writer);

DDS_ReturnCode_t
RTI_Ros2Graph_unregister_local_service(
  RTI_Ros2Graph * const self,
  const RTI_Ros2GraphNodeHandle node_handle,
  DDS_DataReader * const service_reader,
  DDS_DataWriter * const service_writer);

DDS_ReturnCode_t
RTI_Ros2Graph_unregister_local_subscription_by_handle(
  RTI_Ros2Graph * const self,
  const RTI_Ros2GraphNodeHandle node_handle,
  const RTI_Ros2GraphEndpointHandle sub_handle);

DDS_ReturnCode_t
RTI_Ros2Graph_unregister_local_publisher_by_handle(
  RTI_Ros2Graph * const self,
  const RTI_Ros2GraphNodeHandle node_handle,
  const RTI_Ros2GraphEndpointHandle pub_handle);

DDS_ReturnCode_t
RTI_Ros2Graph_unregister_local_client_by_handle(
  RTI_Ros2Graph * const self,
  const RTI_Ros2GraphNodeHandle node_handle,
  const RTI_Ros2GraphEndpointHandle client_handle);

DDS_ReturnCode_t
RTI_Ros2Graph_unregister_local_service_by_handle(
  RTI_Ros2Graph * const self,
  const RTI_Ros2GraphNodeHandle node_handle,
  const RTI_Ros2GraphEndpointHandle service_handle);

#endif  /* rti_ros2_graph_h */