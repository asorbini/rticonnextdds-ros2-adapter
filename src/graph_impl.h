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

#ifndef rti_ros2_graph_impl_h
#define rti_ros2_graph_impl_h

#include "rti/ros2/graph.h"
#include "rmw_dds_common/msg/Graph.h"

#ifndef UNUSED_ARG
#define UNUSED_ARG(x_) (void)(x_)
#endif  /* UNUSED_ARG */

typedef enum RTI_Ros2GraphTopicType
{
  RTI_ROS2_GRAPH_TOPIC_UNKNOWN = 0,
  RTI_ROS2_GRAPH_TOPIC_TOPIC = 1,
  RTI_ROS2_GRAPH_TOPIC_REQUEST = 2,
  RTI_ROS2_GRAPH_TOPIC_REPLY = 3,
} RTI_Ros2GraphTopicType_t;

typedef enum RTI_Ros2GraphEndpointType
{
  RTI_ROS2_GRAPH_ENDPOINT_UNKNOWN = 0,
  RTI_ROS2_GRAPH_ENDPOINT_SUBSCRIPTION = 1,
  RTI_ROS2_GRAPH_ENDPOINT_PUBLISHER = 2,
  RTI_ROS2_GRAPH_ENDPOINT_CLIENT = 3,
  RTI_ROS2_GRAPH_ENDPOINT_SERVICE = 4
} RTI_Ros2GraphEndpointType_t;

typedef struct RTI_Ros2GraphEndpointI
{
  struct REDAInlineListNode list_node;
  RTI_Ros2GraphEndpointHandle handle;
  DDS_DataReader *dds_reader;
  DDS_DataWriter *dds_writer;
  RTI_Ros2GraphEndpointType_t endp_type;
  void * endp_data;
} RTI_Ros2GraphEndpoint;

#define RTI_Ros2GraphEndpoint_INITIALIZER \
{\
  REDAInlineListNode_INITIALIZER /* list_node */,\
  RTI_Ros2GraphEndpointHandle_INVALID /* handle */,\
  NULL /* dds_reader */,\
  NULL /* dds_writer */,\
  RTI_ROS2_GRAPH_ENDPOINT_UNKNOWN /* endp_type */,\
  NULL /* endp_data */\
}

typedef struct RTI_Ros2GraphNodeI
{
  struct REDAInlineListNode list_node;
  RTI_Ros2GraphNodeHandle handle;
  struct REDAInlineList endpoints;
  size_t endpoints_len;
  size_t writers_len;
  size_t readers_len;
  RTI_Ros2GraphEndpointHandle endp_handle_next;
  char * node_name;
  char * node_namespace;
  rmw_dds_common_msg_NodeEntitiesInfo * ninfo;
} RTI_Ros2GraphNode;

#define RTI_Ros2GraphNode_INITIALIZER \
{\
  REDAInlineListNode_INITIALIZER /* list_node */,\
  RTI_Ros2GraphNodeHandle_INVALID /* handle */,\
  REDA_INLINE_LIST_EMPTY /* endpoints */,\
  0 /* endpoints_len */,\
  0 /* writers_len */,\
  0 /* readers_len */,\
  0 /* endp_handle_next */,\
  NULL /* name */,\
  NULL /* node_namespace */,\
  NULL /* ninfo */\
}

struct RTI_Ros2GraphI
{
  struct REDAInlineList nodes;
  size_t nodes_len;
  struct REDAFastBufferPool *nodes_pool;
  struct REDAFastBufferPool *endpoints_pool;
  RTI_Ros2GraphNodeHandle node_handle_next;
  DDS_DomainParticipant * dds_participant;
  DDS_Publisher * dds_publisher;
  DDS_Boolean dds_publisher_own;
  DDS_Topic * graph_topic;
  DDS_Boolean graph_topic_own;
  DDS_DataWriter * graph_writer;
  DDS_Boolean graph_writer_own;
  rmw_dds_common_msg_ParticipantEntitiesInfo * pinfo;
  struct RTIOsapiSemaphore * mutex_self;
  struct RTIOsapiSemaphore * sem_pinfo;
  struct RTIOsapiSemaphore * sem_pinfo_exit;
  struct RTIOsapiThread * thread_pinfo;
  DDS_Boolean thread_pinfo_active;
};

#define RTI_Ros2Graph_INITIALIZER \
{\
  REDA_INLINE_LIST_EMPTY /* nodes */,\
  0 /* nodes_len */,\
  NULL /* nodes_pool */,\
  NULL /* endpoints_pool */,\
  0 /* node_handle_next */,\
  NULL /* dds_participant */,\
  NULL /* dds_publisher */,\
  DDS_BOOLEAN_FALSE /* dds_publisher_own */,\
  NULL /* graph_topic */,\
  DDS_BOOLEAN_FALSE /* graph_topic_own */,\
  NULL /* graph_writer */,\
  DDS_BOOLEAN_FALSE /* graph_writer_own */,\
  NULL /* pinfo */,\
  NULL /* mutex_self */,\
  NULL /* sem_pinfo */,\
  NULL /* sem_pinfo_exit */,\
  NULL /* thread_pinfo */,\
  DDS_BOOLEAN_FALSE /* thread_pinfo_active */\
}

RTI_Ros2GraphNodeHandle
RTI_Ros2Graph_next_node_handle(RTI_Ros2Graph * const self);

RTI_Ros2GraphNodeHandle
RTI_Ros2Graph_next_endpoint_handle(
  RTI_Ros2Graph * const self,
  RTI_Ros2GraphNode * const node);

RTI_Ros2GraphNode*
RTI_Ros2Graph_lookup_local_node_by_name(
  RTI_Ros2Graph * const self,
  const char * const node_name,
  const char * const node_namespace);

RTI_Ros2GraphNode*
RTI_Ros2Graph_lookup_local_node_by_handle(
  RTI_Ros2Graph * const self,
  const RTI_Ros2GraphNodeHandle node_handle);

RTI_Ros2GraphEndpoint*
RTI_Ros2Graph_lookup_local_endpoint_by_handle(
  RTI_Ros2Graph * const self,
  RTI_Ros2GraphNode * const node,
  const RTI_Ros2GraphEndpointHandle endp_handle);

RTI_Ros2GraphEndpoint*
RTI_Ros2Graph_lookup_local_endpoint_by_dds_endpoints(
  RTI_Ros2Graph * const self,
  RTI_Ros2GraphNode * const node,
  DDS_DataReader * const dds_reader,
  DDS_DataWriter * const dds_writer);

RTI_Ros2GraphEndpoint *
RTI_Ros2Graph_register_local_endpoint(
  RTI_Ros2Graph * const self,
  RTI_Ros2GraphNode * const node,
  const RTI_Ros2GraphEndpointType_t endp_type,
  DDS_DataReader * const dds_reader,
  DDS_DataWriter * const dds_writer);

DDS_ReturnCode_t
RTI_Ros2Graph_unregister_local_endpoint(
  RTI_Ros2Graph * const self,
  const RTI_Ros2GraphNodeHandle node_handle,
  DDS_DataReader * const endp_reader,
  DDS_DataWriter * const endp_writer);

DDS_ReturnCode_t
RTI_Ros2Graph_unregister_local_endpoint_by_handle(
  RTI_Ros2Graph * const self,
  const RTI_Ros2GraphNodeHandle node_handle,
  const RTI_Ros2GraphEndpointHandle endp_handle);

DDS_ReturnCode_t
RTI_Ros2Graph_finalize_node(
  RTI_Ros2Graph * const self,
  RTI_Ros2GraphNode * const node);

DDS_ReturnCode_t
RTI_Ros2Graph_finalize_endpoint(
  RTI_Ros2Graph * const self,
  RTI_Ros2GraphNode * const node,
  RTI_Ros2GraphEndpoint * const endp);

void
RTI_Ros2Graph_count_node_endpoints(
  RTI_Ros2Graph * const self,
  RTI_Ros2GraphNode * const node,
  size_t * const readers_count,
  size_t * const writers_count);

void
RTI_Ros2Graph_ih_to_gid(
  const DDS_InstanceHandle_t * const ih,
  rmw_dds_common_msg_Gid * const gid);

DDS_ReturnCode_t
RTI_Ros2Graph_node_to_sample(
  RTI_Ros2Graph * const self,
  RTI_Ros2GraphNode * const node,
  rmw_dds_common_msg_NodeEntitiesInfo * const sample);

DDS_ReturnCode_t
RTI_Ros2Graph_to_sample(
  RTI_Ros2Graph * const self,
  rmw_dds_common_msg_ParticipantEntitiesInfo * const sample);

DDS_ReturnCode_t
RTI_Ros2Graph_publish_update(RTI_Ros2Graph * const self);

void*
RTI_Ros2Graph_update_thread(void * param);

void
RTI_Ros2Graph_queue_update(RTI_Ros2Graph * const self);

#endif  /* rti_ros2_graph_impl_h */