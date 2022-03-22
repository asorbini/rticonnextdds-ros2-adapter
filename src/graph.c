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

#include "graph_impl.h"

#include "rmw_dds_common/msg/GraphSupport.h"

const char * const RTI_ROS2_GRAPH_TOPIC_NAME = "ros_discovery_info";

const char * const RTI_ROS2_GRAPH_TYPE_NAME =
  "rmw_dds_common::msg::dds::ParticipantEntitiesInfo_";

const char * const RTI_ROS2_GRAPH_THREAD_NAME = "ros2-graph";

RTI_Ros2Graph *
RTI_Ros2Graph_new(
  const struct RTI_Ros2GraphProperties * const properties)
{
  DDS_ReturnCode_t retcode = DDS_RETCODE_ERROR;
  RTI_Ros2Graph * result = NULL;
  
  RTIOsapiHeap_allocateStructure(&result, RTI_Ros2Graph);
  if (NULL == result)
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  retcode = RTI_Ros2Graph_initialize(result , properties);
  
done:
  if (DDS_RETCODE_OK != retcode)
  {
    if (NULL != result)
    {
      RTIOsapiHeap_free(result);
      result = NULL;
    }
  }
  return result;
}

DDS_ReturnCode_t
RTI_Ros2Graph_initialize(
  RTI_Ros2Graph * const self,
  const struct RTI_Ros2GraphProperties * const properties)
{
  DDS_ReturnCode_t retcode = DDS_RETCODE_ERROR;
  struct REDAFastBufferPoolProperty pool_props =
        REDA_FAST_BUFFER_POOL_PROPERTY_DEFAULT;
  static const RTI_Ros2Graph def_self = RTI_Ros2Graph_INITIALIZER;

  struct DDS_DataWriterQos graph_writer_qos = DDS_DataWriterQos_INITIALIZER;
  const char * graph_topic_name = NULL;

  if (NULL == properties->dds_participant)
  {
    /* TODO(asorbini) Log error */
    retcode = DDS_RETCODE_BAD_PARAMETER;
    goto done;
  }

  *self = def_self;

  self->nodes_pool =
    REDAFastBufferPool_newForStructure(RTI_Ros2GraphNode, &pool_props);
  if (self->nodes_pool == NULL)
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  self->endpoints_pool =
    REDAFastBufferPool_newForStructure(RTI_Ros2GraphEndpoint, &pool_props);
  if (self->endpoints_pool == NULL)
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  self->dds_participant = properties->dds_participant;

  if (NULL != properties->dds_publisher)
  {
    self->dds_publisher = properties->dds_publisher;
  }
  else
  {
    self->dds_publisher = DDS_DomainParticipant_create_publisher(
      self->dds_participant,
      &DDS_PUBLISHER_QOS_DEFAULT,
      NULL,
      DDS_STATUS_MASK_NONE);
    if (NULL == self->dds_publisher)
    {
      /* TODO(asorbini) Log error */
      goto done;
    }
    self->dds_publisher_own = DDS_BOOLEAN_TRUE;
  }

  if (NULL != properties->graph_topic)
  {
    self->graph_topic = properties->graph_topic;
  }
  else
  {
    if (DDS_RETCODE_OK !=
      rmw_dds_common_msg_ParticipantEntitiesInfoTypeSupport_register_type(
        self->dds_participant, RTI_ROS2_GRAPH_TYPE_NAME))
    {
      /* TODO(asorbini) Log error */
      goto done;
    }

    self->graph_topic = DDS_DomainParticipant_create_topic(
      self->dds_participant,
      RTI_ROS2_GRAPH_TOPIC_NAME,
      RTI_ROS2_GRAPH_TYPE_NAME,
      &DDS_TOPIC_QOS_DEFAULT,
      NULL,
      DDS_STATUS_MASK_NONE);
    if (NULL == self->graph_topic)
    {
      /* TODO(asorbini) Log error */
      goto done;
    }
    self->graph_topic_own = DDS_BOOLEAN_TRUE;
  }

  graph_topic_name = DDS_TopicDescription_get_name(
    DDS_Topic_as_topicdescription(self->graph_topic));
  if (NULL == graph_topic_name)
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  if (DDS_RETCODE_OK !=
    DDS_Publisher_get_default_datawriter_qos_w_topic_name(self->dds_publisher,
      &graph_writer_qos, graph_topic_name))
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  graph_writer_qos.durability.kind = DDS_TRANSIENT_LOCAL_DURABILITY_QOS;
  graph_writer_qos.history.kind = DDS_KEEP_LAST_HISTORY_QOS;
  graph_writer_qos.history.depth = 1;
  graph_writer_qos.reliability.kind = DDS_RELIABLE_RELIABILITY_QOS;
  graph_writer_qos.protocol.rtps_reliable_writer.heartbeat_period.sec = 0;
  graph_writer_qos.protocol.rtps_reliable_writer.heartbeat_period.nanosec = 500000000;
  graph_writer_qos.protocol.rtps_reliable_writer.fast_heartbeat_period.sec = 0;
  graph_writer_qos.protocol.rtps_reliable_writer.fast_heartbeat_period.nanosec = 250000000;
  graph_writer_qos.protocol.rtps_reliable_writer.late_joiner_heartbeat_period.sec = 0;
  graph_writer_qos.protocol.rtps_reliable_writer.late_joiner_heartbeat_period.nanosec = 250000000;
  graph_writer_qos.protocol.rtps_reliable_writer.min_nack_response_delay.sec = 0;
  graph_writer_qos.protocol.rtps_reliable_writer.min_nack_response_delay.nanosec = 0;
  graph_writer_qos.protocol.rtps_reliable_writer.max_nack_response_delay.sec = 0;
  graph_writer_qos.protocol.rtps_reliable_writer.max_nack_response_delay.nanosec = 0;
  graph_writer_qos.protocol.rtps_reliable_writer.nack_suppression_duration.sec = 0;
  graph_writer_qos.protocol.rtps_reliable_writer.nack_suppression_duration.nanosec = 0;
  graph_writer_qos.protocol.rtps_reliable_writer.min_send_window_size = 1;
  graph_writer_qos.protocol.rtps_reliable_writer.max_send_window_size = 1;
  graph_writer_qos.protocol.rtps_reliable_writer.heartbeats_per_max_samples = 1;
  graph_writer_qos.protocol.rtps_reliable_writer.low_watermark = 0;
  graph_writer_qos.protocol.rtps_reliable_writer.high_watermark = 1;
  graph_writer_qos.resource_limits.max_samples = 1;
  graph_writer_qos.resource_limits.initial_samples = 1;
  graph_writer_qos.resource_limits.max_instances = 1;
  graph_writer_qos.resource_limits.initial_instances = 1;
  graph_writer_qos.resource_limits.max_samples_per_instance = 1;
  graph_writer_qos.publish_mode.kind = DDS_ASYNCHRONOUS_PUBLISH_MODE_QOS;
  
  if (DDS_RETCODE_OK !=
    DDS_PropertyQosPolicyHelper_assert_property(
      &graph_writer_qos.property,
      "dds.data_writer.history.memory_manager.fast_pool.pool_buffer_max_size",
      "1024",
      DDS_BOOLEAN_FALSE /* propagate */))
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  self->graph_writer = DDS_Publisher_create_datawriter(self->dds_publisher,
    self->graph_topic,
    &graph_writer_qos,
    NULL,
    DDS_STATUS_MASK_NONE);
  if (NULL == self->graph_writer)
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  self->pinfo =
    rmw_dds_common_msg_ParticipantEntitiesInfoTypeSupport_create_data_ex(
      DDS_BOOLEAN_TRUE);
  if (NULL == self->pinfo)
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  self->mutex_self =
    RTIOsapiSemaphore_new(RTI_OSAPI_SEMAPHORE_KIND_MUTEX, NULL);
  if (NULL == self->mutex_self)
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  self->sem_pinfo =
    RTIOsapiSemaphore_new(RTI_OSAPI_SEMAPHORE_KIND_COUNTING, NULL);
  if (NULL == self->sem_pinfo)
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  self->sem_pinfo_exit =
    RTIOsapiSemaphore_new(RTI_OSAPI_SEMAPHORE_KIND_BINARY, NULL);
  if (NULL == self->sem_pinfo_exit)
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  self->thread_pinfo_active = DDS_BOOLEAN_TRUE;
  self->thread_pinfo =
    RTIOsapiThread_new(
      RTI_ROS2_GRAPH_THREAD_NAME,
      RTI_OSAPI_THREAD_PRIORITY_DEFAULT,
      RTI_OSAPI_THREAD_OPTION_DEFAULT,
      RTI_OSAPI_THREAD_STACK_SIZE_DEFAULT,
      NULL,
      RTI_Ros2Graph_update_thread,
      self);

  retcode = DDS_RETCODE_OK;
  
done:
  if (DDS_RETCODE_OK != retcode)
  {
    if (NULL != self)
    {
      RTI_Ros2Graph_delete(self);
    }
  }
  return retcode;
}

void
RTI_Ros2Graph_delete(RTI_Ros2Graph * const self)
{
  RTI_Ros2Graph_finalize(self);
  RTIOsapiHeap_free(self);
}

DDS_ReturnCode_t
RTI_Ros2Graph_finalize(RTI_Ros2Graph * const self)
{
  DDS_ReturnCode_t retcode = DDS_RETCODE_ERROR;
  RTI_Ros2GraphNode *node =
    (RTI_Ros2GraphNode*) REDAInlineList_getFirst(&self->nodes);

  while (NULL != node)
  {
    RTI_Ros2GraphNode * const next_node =
      (RTI_Ros2GraphNode*) REDAInlineListNode_getNext(&node->list_node);

    if (DDS_RETCODE_OK != RTI_Ros2Graph_finalize_node(self, node))
    {
      /* TODO(asorbini) Log error */
      goto done;
    }
    node = next_node;
  }

  if (NULL != self->thread_pinfo)
  {
    self->thread_pinfo_active = DDS_BOOLEAN_FALSE;
    RTIOsapiSemaphore_give(self->sem_pinfo);
    RTIOsapiSemaphore_take(self->sem_pinfo_exit, RTI_NTP_TIME_INFINITE);
    RTIOsapiThread_delete(self->thread_pinfo);
  }

  if (NULL != self->mutex_self)
  {
    RTIOsapiSemaphore_delete(self->mutex_self);
  }

  if (NULL != self->sem_pinfo)
  {
    RTIOsapiSemaphore_delete(self->sem_pinfo);
  }

  if (NULL != self->sem_pinfo_exit)
  {
    RTIOsapiSemaphore_delete(self->sem_pinfo_exit);
  }

  if (NULL != self->pinfo)
  {
    rmw_dds_common_msg_ParticipantEntitiesInfoTypeSupport_delete_data_ex(self->pinfo, DDS_BOOLEAN_TRUE);
  }

  if (NULL != self->graph_writer && self->graph_writer_own)
  {
    if (DDS_RETCODE_OK !=
      DDS_Publisher_delete_datawriter(self->dds_publisher, self->graph_writer))
    {
      /* TODO(asorbini) Log error */
      goto done;
    }
  }

  if (NULL != self->graph_topic && self->graph_topic_own)
  {
    if (DDS_RETCODE_OK !=
      DDS_DomainParticipant_delete_topic(
        self->dds_participant, self->graph_topic))
    {
      /* TODO(asorbini) Log error */
      goto done;
    }
  }

  if (NULL != self->dds_publisher && self->dds_publisher_own)
  {
    if (DDS_RETCODE_OK !=
      DDS_DomainParticipant_delete_publisher(
        self->dds_participant, self->dds_publisher))
    {
      /* TODO(asorbini) Log error */
      goto done;
    }
  }

  if (NULL != self->nodes_pool)
  {
    REDAFastBufferPool_delete(self->nodes_pool);
  }

  if (NULL != self->endpoints_pool)
  {
    REDAFastBufferPool_delete(self->endpoints_pool);
  }

  retcode = DDS_RETCODE_OK;
done:
  return retcode;
}

RTI_Ros2GraphNodeHandle
RTI_Ros2Graph_register_local_node(
  RTI_Ros2Graph * const self,
  const char * const node_name,
  const char * const node_namespace)
{
  DDS_ReturnCode_t retcode = DDS_RETCODE_ERROR;
  RTI_Ros2GraphNodeHandle result = RTI_Ros2GraphNodeHandle_INVALID;
  RTI_Ros2GraphNode * node = NULL;
  static const RTI_Ros2GraphNode def_node = RTI_Ros2GraphNode_INITIALIZER;

  RTIOsapiSemaphore_take(self->mutex_self, RTI_NTP_TIME_INFINITE);

  /* Check user arguments:
    - must have specified a non-empty name.
    - namespace is optional, but must be non-empty if specified.
    - name and namespace must be within the maximum length defined by
      the IDL data model
  */
  if ((NULL == node_name || node_name[0] == '\0') ||
      (NULL != node_namespace && node_namespace[0] == '\0') ||
      (strlen(node_name) > rmw_dds_common_msg_MAX_NAME_LENGTH) ||
      (strlen(node_namespace) > rmw_dds_common_msg_MAX_NAMESPACE_LENGTH))
  {
    /* TODO(asorbini) Log error */
    retcode = DDS_RETCODE_BAD_PARAMETER;
    goto done;
  }

  node =
    RTI_Ros2Graph_lookup_local_node_by_name(self, node_name, node_namespace);
  if (NULL != node)
  {
    /* TODO(asorbini) Log error */
    node = NULL;
    retcode = DDS_RETCODE_PRECONDITION_NOT_MET;
    goto done;
  }

  result = RTI_Ros2Graph_next_node_handle(self);
  if (RTI_Ros2GraphNodeHandle_INVALID == result)
  {
    /* TODO(asorbini) Log error */
    retcode = DDS_RETCODE_OUT_OF_RESOURCES;
    goto done;
  }

  node = (RTI_Ros2GraphNode*) REDAFastBufferPool_getBuffer(self->nodes_pool);
  if (NULL == node)
  {
    /* TODO(asorbini) Log error */
    retcode = DDS_RETCODE_OUT_OF_RESOURCES;
    goto done;
  }
  *node = def_node;

  REDAInlineList_addNodeToBackEA(&self->nodes, &node->list_node);
  self->nodes_len += 1;

  node->handle = result;
  
  node->node_name = DDS_String_dup(node_name);
  if (NULL == node->node_name)
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  if (NULL != node_namespace)
  {
    node->node_namespace = DDS_String_dup(node_namespace);
    if (NULL == node->node_namespace)
    {
      /* TODO(asorbini) Log error */
      goto done;
    }
  }

  RTI_Ros2Graph_queue_update(self);

  retcode = DDS_RETCODE_OK;
  
done:
  if (DDS_RETCODE_OK != retcode)
  {
    if (NULL != node)
    {
      RTI_Ros2Graph_finalize_node(self, node);
    }
    result = RTI_Ros2GraphNodeHandle_INVALID;
  }
  RTIOsapiSemaphore_give(self->mutex_self);
  return result;
}

RTI_Ros2GraphEndpointHandle
RTI_Ros2Graph_register_local_subscription(
  RTI_Ros2Graph * const self,
  const RTI_Ros2GraphNodeHandle node_handle,
  DDS_DataReader * const sub_reader)
{
  RTI_Ros2GraphEndpointHandle result = RTI_Ros2GraphEndpointHandle_INVALID;
  RTI_Ros2GraphNode * node = NULL;
  RTI_Ros2GraphEndpoint * endp = NULL;

  RTIOsapiSemaphore_take(self->mutex_self, RTI_NTP_TIME_INFINITE);

  node = RTI_Ros2Graph_lookup_local_node_by_handle(self, node_handle);
  if (NULL == node)
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  endp = RTI_Ros2Graph_register_local_endpoint(
    self, node, RTI_ROS2_GRAPH_ENDPOINT_SUBSCRIPTION, sub_reader, NULL);
  if (NULL == endp)
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  RTI_Ros2Graph_queue_update(self);

  result = endp->handle;
  
done:
  if (RTI_Ros2GraphEndpointHandle_INVALID == result)
  {
    if (NULL != endp)
    {
      RTI_Ros2Graph_finalize_endpoint(self, node, endp);
    }
  }
  RTIOsapiSemaphore_give(self->mutex_self);
  return result;
}

RTI_Ros2GraphEndpointHandle
RTI_Ros2Graph_register_local_publication(
  RTI_Ros2Graph * const self,
  const RTI_Ros2GraphNodeHandle node_handle,
  DDS_DataWriter * const pub_writer)
{
  RTI_Ros2GraphEndpointHandle result = RTI_Ros2GraphEndpointHandle_INVALID;
  RTI_Ros2GraphNode * node = NULL;
  RTI_Ros2GraphEndpoint * endp = NULL;

  RTIOsapiSemaphore_take(self->mutex_self, RTI_NTP_TIME_INFINITE);

  node = RTI_Ros2Graph_lookup_local_node_by_handle(self, node_handle);
  if (NULL == node)
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  endp = RTI_Ros2Graph_register_local_endpoint(
    self, node, RTI_ROS2_GRAPH_ENDPOINT_PUBLISHER, NULL, pub_writer);
  if (NULL == endp)
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  RTI_Ros2Graph_queue_update(self);

  result = endp->handle;
  
done:
  if (RTI_Ros2GraphEndpointHandle_INVALID == result)
  {
    if (NULL != endp)
    {
      RTI_Ros2Graph_finalize_endpoint(self, node, endp);
    }
  }
  RTIOsapiSemaphore_give(self->mutex_self);
  return result;
}

RTI_Ros2GraphEndpointHandle
RTI_Ros2Graph_register_local_client(
  RTI_Ros2Graph * const self,
  const RTI_Ros2GraphNodeHandle node_handle,
  DDS_DataWriter * const client_writer,
  DDS_DataReader * const client_reader)
{
  RTI_Ros2GraphEndpointHandle result = RTI_Ros2GraphEndpointHandle_INVALID;
  RTI_Ros2GraphNode * node = NULL;
  RTI_Ros2GraphEndpoint * endp = NULL;

  RTIOsapiSemaphore_take(self->mutex_self, RTI_NTP_TIME_INFINITE);

  node = RTI_Ros2Graph_lookup_local_node_by_handle(self, node_handle);
  if (NULL == node)
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  endp = RTI_Ros2Graph_register_local_endpoint(
    self, node, RTI_ROS2_GRAPH_ENDPOINT_CLIENT, client_reader, client_writer);
  if (NULL == endp)
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  RTI_Ros2Graph_queue_update(self);

  result = endp->handle;
  
done:
  if (RTI_Ros2GraphEndpointHandle_INVALID == result)
  {
    if (NULL != endp)
    {
      RTI_Ros2Graph_finalize_endpoint(self, node, endp);
    }
  }
  RTIOsapiSemaphore_give(self->mutex_self);
  return result;
}

RTI_Ros2GraphEndpointHandle
RTI_Ros2Graph_register_local_service(
  RTI_Ros2Graph * const self,
  const RTI_Ros2GraphNodeHandle node_handle,
  DDS_DataWriter * const service_writer,
  DDS_DataReader * const service_reader)
{
  RTI_Ros2GraphEndpointHandle result = RTI_Ros2GraphEndpointHandle_INVALID;
  RTI_Ros2GraphNode * node = NULL;
  RTI_Ros2GraphEndpoint * endp = NULL;

  RTIOsapiSemaphore_take(self->mutex_self, RTI_NTP_TIME_INFINITE);

  node = RTI_Ros2Graph_lookup_local_node_by_handle(self, node_handle);
  if (NULL == node)
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  endp = RTI_Ros2Graph_register_local_endpoint(
    self, node, RTI_ROS2_GRAPH_ENDPOINT_SERVICE, service_reader, service_writer);
  if (NULL == endp)
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  RTI_Ros2Graph_queue_update(self);

  result = endp->handle;
  
done:
  if (RTI_Ros2GraphEndpointHandle_INVALID == result)
  {
    if (NULL != endp)
    {
      RTI_Ros2Graph_finalize_endpoint(self, node, endp);
    }
  }
  RTIOsapiSemaphore_give(self->mutex_self);
  return result;
}

DDS_ReturnCode_t
RTI_Ros2Graph_unregister_local_node(
  RTI_Ros2Graph * const self,
  const RTI_Ros2GraphNodeHandle node_handle)
{
  DDS_ReturnCode_t retcode = DDS_RETCODE_ERROR;
  RTI_Ros2GraphNode * node = NULL;
  RTI_Ros2GraphEndpoint * endp = NULL;

  RTIOsapiSemaphore_take(self->mutex_self, RTI_NTP_TIME_INFINITE);

  node = RTI_Ros2Graph_lookup_local_node_by_handle(self, node_handle);
  if (NULL == node)
  {
    /* TODO(asorbini) Log error */
    retcode = DDS_RETCODE_BAD_PARAMETER;
    goto done;
  }

  if (DDS_RETCODE_OK != RTI_Ros2Graph_finalize_node(self, node))
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  retcode = DDS_RETCODE_OK;
  
done:
  RTIOsapiSemaphore_give(self->mutex_self);
  return retcode;
}

DDS_ReturnCode_t
RTI_Ros2Graph_unregister_local_subscription(
  RTI_Ros2Graph * const self,
  const RTI_Ros2GraphNodeHandle node_handle,
  DDS_DataReader * const sub_reader)
{
  return RTI_Ros2Graph_unregister_local_endpoint(self, node_handle, sub_reader, NULL);
}

DDS_ReturnCode_t
RTI_Ros2Graph_unregister_local_publisher(
  RTI_Ros2Graph * const self,
  const RTI_Ros2GraphNodeHandle node_handle,
  DDS_DataWriter * const pub_writer)
{
  return RTI_Ros2Graph_unregister_local_endpoint(self, node_handle, NULL, pub_writer);
}

DDS_ReturnCode_t
RTI_Ros2Graph_unregister_local_client(
  RTI_Ros2Graph * const self,
  const RTI_Ros2GraphNodeHandle node_handle,
  DDS_DataReader * const client_reader,
  DDS_DataWriter * const client_writer)
{
  return RTI_Ros2Graph_unregister_local_endpoint(
    self, node_handle, client_reader, client_writer);
}

DDS_ReturnCode_t
RTI_Ros2Graph_unregister_local_service(
  RTI_Ros2Graph * const self,
  const RTI_Ros2GraphNodeHandle node_handle,
  DDS_DataReader * const service_reader,
  DDS_DataWriter * const service_writer)
{
  return RTI_Ros2Graph_unregister_local_endpoint(
    self, node_handle, service_reader, service_writer);
}


DDS_ReturnCode_t
RTI_Ros2Graph_unregister_local_subscription_by_handle(
  RTI_Ros2Graph * const self,
  const RTI_Ros2GraphNodeHandle node_handle,
  const RTI_Ros2GraphEndpointHandle sub_handle)
{
  return RTI_Ros2Graph_unregister_local_endpoint_by_handle(
    self, node_handle, sub_handle);
}

DDS_ReturnCode_t
RTI_Ros2Graph_unregister_local_publisher_by_handle(
  RTI_Ros2Graph * const self,
  const RTI_Ros2GraphNodeHandle node_handle,
  const RTI_Ros2GraphEndpointHandle pub_handle)
{
  return RTI_Ros2Graph_unregister_local_endpoint_by_handle(
    self, node_handle, pub_handle);
}

DDS_ReturnCode_t
RTI_Ros2Graph_unregister_local_client_by_handle(
  RTI_Ros2Graph * const self,
  const RTI_Ros2GraphNodeHandle node_handle,
  const RTI_Ros2GraphEndpointHandle client_handle)
{
  return RTI_Ros2Graph_unregister_local_endpoint_by_handle(
    self, node_handle, client_handle);
}

DDS_ReturnCode_t
RTI_Ros2Graph_unregister_local_service_by_handle(
  RTI_Ros2Graph * const self,
  const RTI_Ros2GraphNodeHandle node_handle,
  const RTI_Ros2GraphEndpointHandle service_handle)
{
  return RTI_Ros2Graph_unregister_local_endpoint_by_handle(
    self, node_handle, service_handle);
}

