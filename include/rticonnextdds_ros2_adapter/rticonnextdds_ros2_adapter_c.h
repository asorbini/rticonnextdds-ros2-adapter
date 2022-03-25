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

#ifndef RTICONNEXTDDS_ROS2_ADAPTER__C_H
#define RTICONNEXTDDS_ROS2_ADAPTER__C_H

#include "ndds/ndds_c.h"
#include "rticonnextdds_ros2_adapter/rticonnextdds_ros2_adapter_dll.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/* The generated type support does NOT and "extern C" declaration so we
   must wrap the includes for correct linkage */
#include "rticonnextdds_ros2_adapter/rticonnextdds_ros2_adapter_model.h"
#include "rticonnextdds_ros2_adapter/rticonnextdds_ros2_adapter_modelSupport.h"

RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
extern const char * const RTIROS2_GRAPH_TOPIC_NAME;

RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
extern const char * const RTIROS2_GRAPH_TYPE_NAME;

RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
extern const char * const RTIROS2_TOPIC_PREFIX_DEFAULT;

RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
extern const char * const RTIROS2_TOPIC_PREFIX_REQUEST;

RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
extern const char * const RTIROS2_TOPIC_PREFIX_RESPONSE;

RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
extern const char * const RTIROS2_TYPE_NAMESPACE;

RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
extern const char * const RTIROS2_TYPE_SUFFIX_DEFAULT;

RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
extern const char * const RTIROS2_TYPE_SUFFIX_REQUEST;

RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
extern const char * const RTIROS2_TYPE_SUFFIX_RESPONSE;


typedef struct RTIROS2_GraphI RTIROS2_Graph;
typedef DDS_Long RTIROS2_GraphNodeHandle;
typedef DDS_Long RTIROS2_GraphEndpointHandle;

#define RTIROS2_GraphNodeHandle_INVALID (-1)
#define RTIROS2_GraphEndpointHandle_INVALID (-1)

typedef enum RTIROS2_GraphEndpointType
{
  RTIROS2_GRAPH_ENDPOINT_UNKNOWN = 0,
  RTIROS2_GRAPH_ENDPOINT_SUBSCRIPTION = 1,
  RTIROS2_GRAPH_ENDPOINT_PUBLISHER = 2,
  RTIROS2_GRAPH_ENDPOINT_CLIENT = 3,
  RTIROS2_GRAPH_ENDPOINT_SERVICE = 4
} RTIROS2_GraphEndpointType_t;

typedef DDS_ReturnCode_t (*RTIROS2_GraphSampleAdapter_ConvertToSampleFn)(
  RTIROS2_Graph * const graph,
  DDS_DomainParticipant * const dds_participant,
  void * const sample,
  void * const adapter_data);

typedef DDS_ReturnCode_t (*RTIROS2_GraphSampleAdapter_PublishSampleFn)(
  RTIROS2_Graph * const graph,
  void * const sample,
  void * const adapter_data);

typedef void * (*RTIROS2_GraphSampleAdapter_AllocSampleFn)(
  RTIROS2_Graph * const graph,
  void * const adapter_data);

typedef void (*RTIROS2_GraphSampleAdapter_FreeSampleFn)(
  RTIROS2_Graph * const graph,
  void * const sample,
  void * const adapter_data);

typedef struct RTIROS2_GraphSampleAdapterI
{
  RTIROS2_GraphSampleAdapter_ConvertToSampleFn convert_to_sample;
  RTIROS2_GraphSampleAdapter_PublishSampleFn publish_sample;
  RTIROS2_GraphSampleAdapter_AllocSampleFn alloc_sample;
  RTIROS2_GraphSampleAdapter_FreeSampleFn free_sample;
  void * adapter_data;
} RTIROS2_GraphSampleAdapter;

#define RTIROS2_GraphSampleAdapter_INITIALIZER \
{\
  NULL /* convert_to_sample */,\
  NULL /* publish_sample */,\
  NULL /* alloc_sample */,\
  NULL /* free_sample */,\
  NULL /* adapter_data */\
}

#define RTIROS2_GraphSampleAdapter_is_valid(s_) \
  ((NULL != (s_)->convert_to_sample) && \
    (NULL != (s_)->publish_sample) && \
    (NULL != (s_)->alloc_sample) &&\
    (NULL != (s_)->free_sample))

struct RTIROS2_GraphProperties
{
  DDS_DomainParticipant * graph_participant;
  DDS_Publisher * graph_publisher;
  DDS_Topic * graph_topic;
  DDS_DataWriter * graph_writer;
  struct DDS_Duration_t poll_period;
  RTIROS2_GraphSampleAdapter sample_adapter;
};

#define RTIROS2_GraphProperties_INITIALIZER \
{\
  NULL /* graph_participant */,\
  NULL /* graph_publisher */,\
  NULL /* graph_topic */,\
  NULL /* graph_writer */,\
  {0, 0} /* poll_period */,\
  RTIROS2_GraphSampleAdapter_INITIALIZER /* sample_adapter */\
}

RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
DDS_ReturnCode_t
RTIROS2_Graph_customize_datawriter_qos(
  struct DDS_DataWriterQos * const writer_qos);

RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
DDS_ReturnCode_t
RTIROS2_Graph_customize_datareader_qos(
  struct DDS_DataReaderQos * const reader_qos);

RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
RTIROS2_Graph *
RTIROS2_Graph_new(
  const struct RTIROS2_GraphProperties * const properties);

RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
void
RTIROS2_Graph_delete(RTIROS2_Graph * const self);

RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
DDS_DomainParticipant*
RTIROS2_Graph_get_graph_participant(RTIROS2_Graph * const self);

RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
DDS_Publisher*
RTIROS2_Graph_get_graph_publisher(RTIROS2_Graph * const self);

RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
DDS_Topic*
RTIROS2_Graph_get_graph_topic(RTIROS2_Graph * const self);

RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
DDS_DataWriter*
RTIROS2_Graph_get_graph_writer(RTIROS2_Graph * const self);

RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
RTIROS2_GraphNodeHandle
RTIROS2_Graph_register_local_node(
  RTIROS2_Graph * const self,
  const char * const node_name,
  const char * const node_namespace,
  DDS_DomainParticipant * const dds_participant);

RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
DDS_ReturnCode_t
RTIROS2_Graph_inspect_local_node(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle);

RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
RTIROS2_GraphEndpointHandle
RTIROS2_Graph_register_local_subscription(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle,
  DDS_DataReader * const sub_reader);

RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
RTIROS2_GraphEndpointHandle
RTIROS2_Graph_register_local_publisher(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle,
  DDS_DataWriter * const pub_writer);

RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
RTIROS2_GraphEndpointHandle
RTIROS2_Graph_register_local_client(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle,
  DDS_DataReader * const client_reader,
  DDS_DataWriter * const client_writer);

RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
RTIROS2_GraphEndpointHandle
RTIROS2_Graph_register_local_service(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle,
  DDS_DataReader * const service_reader,
  DDS_DataWriter * const service_writer);

RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
DDS_ReturnCode_t
RTIROS2_Graph_unregister_local_node(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle);

RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
DDS_ReturnCode_t
RTIROS2_Graph_unregister_local_subscription(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle,
  DDS_DataReader * const sub_reader);

RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
DDS_ReturnCode_t
RTIROS2_Graph_unregister_local_publisher(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle,
  DDS_DataWriter * const pub_writer);

RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
DDS_ReturnCode_t
RTIROS2_Graph_unregister_local_client(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle,
  DDS_DataReader * const client_reader,
  DDS_DataWriter * const client_writer);

RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
DDS_ReturnCode_t
RTIROS2_Graph_unregister_local_service(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle,
  DDS_DataReader * const service_reader,
  DDS_DataWriter * const service_writer);

RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
DDS_ReturnCode_t
RTIROS2_Graph_unregister_local_subscription_by_handle(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle,
  const RTIROS2_GraphEndpointHandle sub_handle);

RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
DDS_ReturnCode_t
RTIROS2_Graph_unregister_local_publisher_by_handle(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle,
  const RTIROS2_GraphEndpointHandle pub_handle);

RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
DDS_ReturnCode_t
RTIROS2_Graph_unregister_local_client_by_handle(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle,
  const RTIROS2_GraphEndpointHandle client_handle);

RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
DDS_ReturnCode_t
RTIROS2_Graph_unregister_local_service_by_handle(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle,
  const RTIROS2_GraphEndpointHandle service_handle);

RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
DDS_DomainParticipant*
RTIROS2_Graph_get_node_participant(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle);

RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
DDS_ReturnCode_t
RTIROS2_Graph_get_node_name(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle,
  const char ** const name_out,
  const char ** const namespace_out);

RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
DDS_DataReader*
RTIROS2_Graph_get_endpoint_reader(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle,
  const RTIROS2_GraphEndpointHandle endp_handle);

RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
DDS_DataWriter*
RTIROS2_Graph_get_endpoint_writer(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle,
  const RTIROS2_GraphEndpointHandle endp_handle);

RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
RTIROS2_GraphEndpointType_t
RTIROS2_Graph_get_endpoint_type(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle,
  const RTIROS2_GraphEndpointHandle endp_handle);

RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
DDS_ReturnCode_t
RTIROS2_Graph_compute_participant_gid(
  DDS_DomainParticipant * const dds_participant, RTIROS2_Gid * const gid);

RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
DDS_ReturnCode_t
RTIROS2_Graph_compute_reader_gid(
  DDS_DataReader * const dds_reader, RTIROS2_Gid * const gid);

RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
DDS_ReturnCode_t
RTIROS2_Graph_compute_writer_gid(
  DDS_DataWriter * const dds_writer, RTIROS2_Gid * const gid);

RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
DDS_ReturnCode_t
RTIROS2_Graph_compute_publisher_topic_names(
  const char * const ros2_topic_name,
  const char * const ros2_type_name,
  char * const dds_topic_name,
  size_t * const dds_topic_name_len,
  char * const dds_type_name,
  size_t * const dds_type_name_len);

RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
DDS_ReturnCode_t
RTIROS2_Graph_compute_subscription_topic_names(
  const char * const ros2_topic_name,
  const char * const ros2_type_name,
  char * const dds_topic_name,
  size_t * const dds_topic_name_len,
  char * const dds_type_name,
  size_t * const dds_type_name_len);

RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
DDS_ReturnCode_t
RTIROS2_Graph_compute_service_topic_names(
  const char * const ros2_node_name,
  const char * const ros2_service_name,
  const char * const ros2_type_name,
  char * const dds_req_topic_name,
  size_t * const dds_req_topic_name_len,
  char * const dds_req_type_name,
  size_t * const dds_req_type_name_len,
  char * const dds_rep_topic_name,
  size_t * const dds_rep_topic_name_len,
  char * const dds_rep_type_name,
  size_t * const dds_rep_type_name_len);

RTICONNEXTDDS_ROS2_ADAPTER_PUBLIC
DDS_ReturnCode_t
RTIROS2_CGraphSampleAdapter_convert_to_sample(
  RTIROS2_Graph * const self,
  DDS_DomainParticipant * const dds_participant,
  void * const sample,
  void * const adapter_data);

#ifdef __cplusplus
}  /* extern "C" */
#endif  /* __cplusplus */

#endif  /* RTICONNEXTDDS_ROS2_ADAPTER__C_H */