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

#include "rticonnextdds_ros2_adapter/rticonnextdds_ros2_adapter_c.h"

#ifndef UNUSED_ARG
#define UNUSED_ARG(arg_)        (void)(arg_)
#endif /* UNUSED_ARG */

extern const char * const RTIROS2_GRAPH_THREAD_NAME;

typedef enum RTIROS2_GraphTopicType
{
  RTIROS2_GRAPH_TOPIC_UNKNOWN = 0,
  RTIROS2_GRAPH_TOPIC_TOPIC = 1,
  RTIROS2_GRAPH_TOPIC_REQUEST = 2,
  RTIROS2_GRAPH_TOPIC_REPLY = 3,
} RTIROS2_GraphTopicType_t;

typedef struct RTIROS2_GraphEndpointI
{
  struct REDAInlineListNode list_node;
  RTIROS2_GraphEndpointHandle handle;
  DDS_DataReader *dds_reader;
  DDS_DataWriter *dds_writer;
  RTIROS2_GraphEndpointType_t endp_type;
  void * endp_data;
} RTIROS2_GraphEndpoint;

#define RTIROS2_GraphEndpoint_INITIALIZER \
{\
  REDAInlineListNode_INITIALIZER /* list_node */,\
  RTIROS2_GraphEndpointHandle_INVALID /* handle */,\
  NULL /* dds_reader */,\
  NULL /* dds_writer */,\
  RTIROS2_GRAPH_ENDPOINT_UNKNOWN /* endp_type */,\
  NULL /* endp_data */\
}

typedef struct RTIROS2_GraphNodeI
{
  struct REDAInlineListNode list_node;
  RTIROS2_GraphNodeHandle handle;
  struct REDAInlineList endpoints;
  size_t endpoints_len;
  size_t writers_len;
  size_t readers_len;
  RTIROS2_GraphEndpointHandle endp_handle_next;
  char * node_name;
  char * node_namespace;
  RTIROS2_NodeEntitiesInfo * ninfo;
  DDS_DomainParticipant * dds_participant;
} RTIROS2_GraphNode;

#define RTIROS2_GraphNode_INITIALIZER \
{\
  REDAInlineListNode_INITIALIZER /* list_node */,\
  RTIROS2_GraphNodeHandle_INVALID /* handle */,\
  REDA_INLINE_LIST_EMPTY /* endpoints */,\
  0 /* endpoints_len */,\
  0 /* writers_len */,\
  0 /* readers_len */,\
  0 /* endp_handle_next */,\
  NULL /* name */,\
  NULL /* node_namespace */,\
  NULL /* ninfo */,\
  NULL /* dds_participant */\
}

struct RTIROS2_GraphI
{
  struct REDAInlineList nodes;
  size_t nodes_len;
  struct REDAFastBufferPool *nodes_pool;
  struct REDAFastBufferPool *endpoints_pool;
  RTIROS2_GraphNodeHandle node_handle_next;
  DDS_DomainParticipant * graph_participant;
  DDS_Publisher * graph_publisher;
  DDS_Boolean graph_publisher_own;
  DDS_Topic * graph_topic;
  DDS_Boolean graph_topic_own;
  DDS_DataWriter * graph_writer;
  DDS_Boolean graph_writer_own;
  RTIROS2_ParticipantEntitiesInfo * pinfo;
  struct RTIOsapiSemaphore * mutex_self;
  struct RTIOsapiSemaphore * sem_pinfo;
  struct RTIOsapiSemaphore * sem_pinfo_exit;
  struct RTIOsapiThread * thread_pinfo;
  DDS_Boolean thread_pinfo_active;
  struct DDS_Duration_t poll_period;
  RTIROS2_GraphSampleAdapter sample_adapter;
  struct DDS_DomainParticipantSeq participants;
};

#define RTIROS2_Graph_INITIALIZER \
{\
  REDA_INLINE_LIST_EMPTY /* nodes */,\
  0 /* nodes_len */,\
  NULL /* nodes_pool */,\
  NULL /* endpoints_pool */,\
  0 /* node_handle_next */,\
  NULL /* graph_participant */,\
  NULL /* graph_publisher */,\
  DDS_BOOLEAN_FALSE /* graph_publisher_own */,\
  NULL /* graph_topic */,\
  DDS_BOOLEAN_FALSE /* graph_topic_own */,\
  NULL /* graph_writer */,\
  DDS_BOOLEAN_FALSE /* graph_writer_own */,\
  NULL /* pinfo */,\
  NULL /* mutex_self */,\
  NULL /* sem_pinfo */,\
  NULL /* sem_pinfo_exit */,\
  NULL /* thread_pinfo */,\
  DDS_BOOLEAN_FALSE /* thread_pinfo_active */,\
  {0, 0} /* poll_period */,\
  RTIROS2_GraphSampleAdapter_INITIALIZER /* sample_adapter */,\
  DDS_SEQUENCE_INITIALIZER /* participants */\
}

DDS_ReturnCode_t
RTIROS2_Graph_initialize(
  RTIROS2_Graph * const self,
  const struct RTIROS2_GraphProperties * const properties);

DDS_ReturnCode_t
RTIROS2_Graph_finalize(RTIROS2_Graph * const self);

#define RTIROS2_Graph_is_polling_enabled(s_) \
  (!DDS_Duration_is_zero(&(s_)->poll_period) && \
    !DDS_Duration_is_infinite(&(s_)->poll_period))

RTIROS2_GraphEndpointType_t
RTIROS2_Graph_detect_endpoint_type(
  const char * const topic_name,
  const DDS_Boolean writer);

DDS_ReturnCode_t
RTIROS2_Graph_get_graph_writer_qos(
  RTIROS2_Graph * const self,
  struct DDS_DataWriterQos * const writer_qos);

RTIROS2_GraphNodeHandle
RTIROS2_Graph_next_node_handle(RTIROS2_Graph * const self);

RTIROS2_GraphNodeHandle
RTIROS2_Graph_next_endpoint_handle(
  RTIROS2_Graph * const self,
  RTIROS2_GraphNode * const node);

RTIROS2_GraphNode*
RTIROS2_Graph_lookup_local_node_by_name(
  RTIROS2_Graph * const self,
  const char * const node_name,
  const char * const node_namespace);

RTIROS2_GraphNode*
RTIROS2_Graph_lookup_local_node_by_handle(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle);

RTIROS2_GraphEndpoint*
RTIROS2_Graph_lookup_local_endpoint_by_handle(
  RTIROS2_Graph * const self,
  RTIROS2_GraphNode * const node,
  const RTIROS2_GraphEndpointHandle endp_handle);

RTIROS2_GraphEndpoint*
RTIROS2_Graph_lookup_local_endpoint_by_dds_endpoints(
  RTIROS2_Graph * const self,
  RTIROS2_GraphNode * const node,
  DDS_DataReader * const dds_reader,
  DDS_DataWriter * const dds_writer);

RTIROS2_GraphEndpoint*
RTIROS2_Graph_lookup_local_endpoint_by_topic_name(
  RTIROS2_Graph * const self,
  RTIROS2_GraphNode * const node,
  const char * const topic_name,
  const DDS_Boolean writer);

RTIROS2_GraphEndpoint *
RTIROS2_Graph_register_local_endpoint(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle,
  const RTIROS2_GraphEndpointType_t endp_type,
  DDS_DataReader * const dds_reader,
  DDS_DataWriter * const dds_writer);

DDS_ReturnCode_t
RTIROS2_Graph_unregister_local_endpoint(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle,
  DDS_DataReader * const endp_reader,
  DDS_DataWriter * const endp_writer);

DDS_ReturnCode_t
RTIROS2_Graph_unregister_local_endpoint_by_handle(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle,
  const RTIROS2_GraphEndpointHandle endp_handle);

DDS_ReturnCode_t
RTIROS2_Graph_finalize_node(
  RTIROS2_Graph * const self,
  RTIROS2_GraphNode * const node);

DDS_ReturnCode_t
RTIROS2_Graph_finalize_endpoint(
  RTIROS2_Graph * const self,
  RTIROS2_GraphNode * const node,
  RTIROS2_GraphEndpoint * const endp);

void
RTIROS2_Graph_count_node_endpoints(
  RTIROS2_Graph * const self,
  RTIROS2_GraphNode * const node,
  size_t * const readers_count,
  size_t * const writers_count);

void
RTIROS2_Graph_ih_to_gid(
  const DDS_InstanceHandle_t * const ih,
  RTIROS2_Gid * const gid);

DDS_ReturnCode_t
RTIROS2_Graph_node_to_sample(
  RTIROS2_Graph * const self,
  RTIROS2_GraphNode * const node,
  RTIROS2_NodeEntitiesInfo * const sample);

DDS_ReturnCode_t
RTIROS2_CGraphSampleAdapter_publish_sample(
  RTIROS2_Graph * const self,
  void * const sample,
  void * const adapter_data);

void*
RTIROS2_CGraphSampleAdapter_alloc_sample(
  RTIROS2_Graph * const self,
  void * const adapter_data);

void
RTIROS2_CGraphSampleAdapter_free_sample(
  RTIROS2_Graph * const self,
  void * const sample,
  void * const adapter_data);

DDS_ReturnCode_t
RTIROS2_Graph_publish_update(RTIROS2_Graph * const self);

void*
RTIROS2_Graph_update_thread(void * param);

void
RTIROS2_Graph_queue_update(RTIROS2_Graph * const self);


DDS_ReturnCode_t
RTIROS2_Graph_inspect_local_nodeEA(
  RTIROS2_Graph * const self,
  RTIROS2_GraphNode * const node);


DDS_ReturnCode_t
RTIROS2_Graph_convert_to_sample(
  RTIROS2_Graph * const self,
  DDS_DomainParticipant * const dds_participant,
  void * const sample);

#define RTIROS2_Graph_convert_to_sample(s_, dp_, ss_) \
  ((s_)->sample_adapter.convert_to_sample((s_), (dp_), (ss_),\
    (s_)->sample_adapter.adapter_data))

DDS_ReturnCode_t
RTIROS2_Graph_publish_sample(
  RTIROS2_Graph * const self,
  void * const sample);

#define RTIROS2_Graph_publish_sample(s_, ss_) \
  ((s_)->sample_adapter.publish_sample((s_), (ss_),\
    (s_)->sample_adapter.adapter_data))

void*
RTIROS2_Graph_alloc_sample(
  RTIROS2_Graph * const self);

#define RTIROS2_Graph_alloc_sample(s_) \
  ((s_)->sample_adapter.alloc_sample((s_),\
    (s_)->sample_adapter.adapter_data))

void
RTIROS2_Graph_free_sample(
  RTIROS2_Graph * const self,
  void * const sample);

#define RTIROS2_Graph_free_sample(s_, ss_) \
  ((s_)->sample_adapter.free_sample((s_), (ss_),\
    (s_)->sample_adapter.adapter_data))

DDS_ReturnCode_t
RTIROS2_Graph_gather_domain_participants(
  RTIROS2_Graph * const self,
  struct DDS_DomainParticipantSeq * const participants);

RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
DDS_ReturnCode_t
RTIROS2_Graph_compute_dds_writer_topic_names(
  const char * const ros2_topic_name,
  const char * const ros2_type_name,
  const RTIROS2_GraphEndpointType_t ros2_endp_type,
  char * const dds_topic_name,
  size_t * const dds_topic_name_len,
  char * const dds_type_name,
  size_t * const dds_type_name_len);

RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
DDS_ReturnCode_t
RTIROS2_Graph_compute_dds_reader_topic_names(
  const char * const ros2_topic_name,
  const char * const ros2_type_name,
  const RTIROS2_GraphEndpointType_t ros2_endp_type,
  char * const dds_topic_name,
  size_t * const dds_topic_name_len,
  char * const dds_type_name,
  size_t * const dds_type_name_len);

#endif  /* rti_ros2_graph_impl_h */