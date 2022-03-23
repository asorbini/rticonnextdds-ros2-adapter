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

const char * const RTIROS2_GRAPH_TOPIC_NAME = "ros_discovery_info";

const char * const RTIROS2_GRAPH_TYPE_NAME =
  "rmw_dds_common::msg::dds_::ParticipantEntitiesInfo_";

DDS_ReturnCode_t
RTIROS2_Graph_customize_datawriter_qos(
  struct DDS_DataWriterQos * const writer_qos)
{
  DDS_ReturnCode_t retcode = DDS_RETCODE_ERROR;

  if (NULL == writer_qos)
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  writer_qos->durability.kind = DDS_TRANSIENT_LOCAL_DURABILITY_QOS;
  writer_qos->history.kind = DDS_KEEP_LAST_HISTORY_QOS;
  writer_qos->history.depth = 1;
  writer_qos->reliability.kind = DDS_RELIABLE_RELIABILITY_QOS;
  writer_qos->protocol.rtps_reliable_writer.heartbeat_period.sec = 0;
  writer_qos->protocol.rtps_reliable_writer.heartbeat_period.nanosec = 500000000;
  writer_qos->protocol.rtps_reliable_writer.fast_heartbeat_period.sec = 0;
  writer_qos->protocol.rtps_reliable_writer.fast_heartbeat_period.nanosec = 250000000;
  writer_qos->protocol.rtps_reliable_writer.late_joiner_heartbeat_period.sec = 0;
  writer_qos->protocol.rtps_reliable_writer.late_joiner_heartbeat_period.nanosec = 250000000;
  writer_qos->protocol.rtps_reliable_writer.min_nack_response_delay.sec = 0;
  writer_qos->protocol.rtps_reliable_writer.min_nack_response_delay.nanosec = 0;
  writer_qos->protocol.rtps_reliable_writer.max_nack_response_delay.sec = 0;
  writer_qos->protocol.rtps_reliable_writer.max_nack_response_delay.nanosec = 0;
  writer_qos->protocol.rtps_reliable_writer.nack_suppression_duration.sec = 0;
  writer_qos->protocol.rtps_reliable_writer.nack_suppression_duration.nanosec = 0;
  writer_qos->protocol.rtps_reliable_writer.min_send_window_size = 1;
  writer_qos->protocol.rtps_reliable_writer.max_send_window_size = 1;
  writer_qos->protocol.rtps_reliable_writer.heartbeats_per_max_samples = 1;
  writer_qos->protocol.rtps_reliable_writer.low_watermark = 0;
  writer_qos->protocol.rtps_reliable_writer.high_watermark = 1;
  writer_qos->resource_limits.max_samples = 1;
  writer_qos->resource_limits.initial_samples = 1;
  writer_qos->resource_limits.max_instances = 1;
  writer_qos->resource_limits.initial_instances = 1;
  writer_qos->resource_limits.max_samples_per_instance = 1;
  // writer_qos->publish_mode.kind = DDS_ASYNCHRONOUS_PUBLISH_MODE_QOS;
  
  if (DDS_RETCODE_OK !=
    DDS_PropertyQosPolicyHelper_assert_property(
      &writer_qos->property,
      "dds.data_writer.history.memory_manager.fast_pool.pool_buffer_max_size",
      "1024",
      DDS_BOOLEAN_FALSE /* propagate */))
  {
    /* TODO(asorbini) Log error */
    goto done;
  }
  
  retcode = DDS_RETCODE_OK;
  
done:
  return retcode;
}

DDS_ReturnCode_t
RTIROS2_Graph_customize_datareader_qos(
  struct DDS_DataReaderQos * const reader_qos)
{
  DDS_ReturnCode_t retcode = DDS_RETCODE_ERROR;

  if (NULL == reader_qos)
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  reader_qos->durability.kind = DDS_TRANSIENT_LOCAL_DURABILITY_QOS;
  reader_qos->history.kind = DDS_KEEP_LAST_HISTORY_QOS;
  reader_qos->history.depth = 1;
  reader_qos->reliability.kind = DDS_RELIABLE_RELIABILITY_QOS;
  
  reader_qos->protocol.rtps_reliable_reader.min_heartbeat_response_delay.sec = 0;
  reader_qos->protocol.rtps_reliable_reader.min_heartbeat_response_delay.nanosec = 0;
  reader_qos->protocol.rtps_reliable_reader.max_heartbeat_response_delay.sec = 0;
  reader_qos->protocol.rtps_reliable_reader.max_heartbeat_response_delay.nanosec = 0;
  reader_qos->protocol.rtps_reliable_reader.heartbeat_suppression_duration.sec = 0;
  reader_qos->protocol.rtps_reliable_reader.heartbeat_suppression_duration.nanosec = 0;

  if (DDS_RETCODE_OK !=
    DDS_PropertyQosPolicyHelper_assert_property(
      &reader_qos->property,
      "dds.data_reader.history.memory_manager.fast_pool.pool_buffer_max_size",
      "1024",
      DDS_BOOLEAN_FALSE /* propagate */))
  {
    /* TODO(asorbini) Log error */
    goto done;
  }
  
  retcode = DDS_RETCODE_OK;
  
done:
  return retcode;
}

RTIROS2_Graph *
RTIROS2_Graph_new(
  const struct RTIROS2_GraphProperties * const properties)
{
  DDS_ReturnCode_t retcode = DDS_RETCODE_ERROR;
  RTIROS2_Graph * result = NULL;

  if (NULL == properties)
  {
    /* TODO(asorbini) Log error */
    goto done;
  }
  
  RTIOsapiHeap_allocateStructure(&result, RTIROS2_Graph);
  if (NULL == result)
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  retcode = RTIROS2_Graph_initialize(result , properties);
  
done:
  if (DDS_RETCODE_OK != retcode)
  {
    if (NULL != result)
    {
      RTIOsapiHeap_freeStructure(result);
      result = NULL;
    }
  }
  return result;
}

void
RTIROS2_Graph_delete(RTIROS2_Graph * const self)
{
  if (NULL == self)
  {
    /* TODO(asorbini) Log error */
    return;
  }
  RTIROS2_Graph_finalize(self);
  RTIOsapiHeap_free(self);
}

DDS_DomainParticipant*
RTIROS2_Graph_get_graph_participant(RTIROS2_Graph * const self)
{
  if (NULL == self)
  {
    return NULL;
  }
  return self->graph_participant;
}

DDS_Publisher*
RTIROS2_Graph_get_graph_publisher(RTIROS2_Graph * const self)
{
  if (NULL == self)
  {
    return NULL;
  }
  return self->graph_publisher;
}

DDS_Topic*
RTIROS2_Graph_get_graph_topic(RTIROS2_Graph * const self)
{
  if (NULL == self)
  {
    return NULL;
  }
  return self->graph_topic;
}

DDS_DataWriter*
RTIROS2_Graph_get_graph_writer(RTIROS2_Graph * const self)
{
  if (NULL == self)
  {
    return NULL;
  }
  return self->graph_writer;
}

RTIROS2_GraphNodeHandle
RTIROS2_Graph_register_local_node(
  RTIROS2_Graph * const self,
  const char * const node_name,
  const char * const node_namespace,
  DDS_DomainParticipant * const dds_participant)
{
  DDS_ReturnCode_t retcode = DDS_RETCODE_ERROR;
  RTIROS2_GraphNodeHandle result = RTIROS2_GraphNodeHandle_INVALID;
  RTIROS2_GraphNode * node = NULL;
  static const RTIROS2_GraphNode def_node = RTIROS2_GraphNode_INITIALIZER;
  DDS_Boolean enable_poll = DDS_BOOLEAN_FALSE;

  if (NULL == self)
  {
    return RTIROS2_GraphNodeHandle_INVALID;
  }

  /* Check user arguments:
    - must have specified a non-empty name.
    - namespace is optional, but must be non-empty if specified.
    - name and namespace must be within the maximum length defined by
      the IDL data model
  */
  if ((NULL == node_name || node_name[0] == '\0' ||
      strlen(node_name) > RTIROS2_MAX_NAME_LENGTH) ||
    (NULL != node_namespace &&
      (node_namespace[0] == '\0' ||
      strlen(node_namespace) > RTIROS2_MAX_NAMESPACE_LENGTH)))
  {
    /* TODO(asorbini) Log error */
    return RTIROS2_GraphNodeHandle_INVALID;
  }

  RTIOsapiSemaphore_take(self->mutex_self, RTI_NTP_TIME_INFINITE);

  enable_poll = RTIROS2_Graph_is_polling_enabled(self);

  node =
    RTIROS2_Graph_lookup_local_node_by_name(self, node_name, node_namespace);
  if (NULL != node)
  {
    /* TODO(asorbini) Log error */
    node = NULL;
    retcode = DDS_RETCODE_PRECONDITION_NOT_MET;
    goto done;
  }

  result = RTIROS2_Graph_next_node_handle(self);
  if (RTIROS2_GraphNodeHandle_INVALID == result)
  {
    /* TODO(asorbini) Log error */
    retcode = DDS_RETCODE_OUT_OF_RESOURCES;
    goto done;
  }

  node = (RTIROS2_GraphNode*) REDAFastBufferPool_getBuffer(self->nodes_pool);
  if (NULL == node)
  {
    /* TODO(asorbini) Log error */
    retcode = DDS_RETCODE_OUT_OF_RESOURCES;
    goto done;
  }
  *node = def_node;

  REDAInlineList_addNodeToBackEA(&self->nodes, &node->list_node);
  self->nodes_len += 1;

  node->dds_participant =
    (NULL == dds_participant)?self->graph_participant:dds_participant;

  node->handle = result;
  
  node->node_name = DDS_String_dup(node_name);
  if (NULL == node->node_name)
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  node->node_namespace = DDS_String_dup(
    (NULL != node_namespace)?node_namespace:"/");
  if (NULL == node->node_namespace)
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  RTIROS2_Graph_queue_update(self);

  if (enable_poll)
  {
    if (DDS_RETCODE_OK != RTIROS2_Graph_inspect_local_nodeEA(self, node))
    {
      /* TODO(asorbini) Log error */
      goto done;
    }
  }

  retcode = DDS_RETCODE_OK;
  
done:
  if (DDS_RETCODE_OK != retcode)
  {
    if (NULL != node)
    {
      RTIROS2_Graph_finalize_node(self, node);
    }
    result = RTIROS2_GraphNodeHandle_INVALID;
  }
  RTIOsapiSemaphore_give(self->mutex_self);
  return result;
}

DDS_ReturnCode_t
RTIROS2_Graph_inspect_local_node(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle)
{
  DDS_ReturnCode_t retcode = DDS_RETCODE_ERROR;
  RTIROS2_GraphNode * node = NULL;

  if (NULL == self)
  {
    /* TODO(asorbini) Log error */
    retcode = DDS_RETCODE_BAD_PARAMETER;
    goto done;
  }

  RTIOsapiSemaphore_take(self->mutex_self, RTI_NTP_TIME_INFINITE);

  node = RTIROS2_Graph_lookup_local_node_by_handle(self, node_handle);
  if (NULL == node)
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  retcode = RTIROS2_Graph_inspect_local_nodeEA(self, node);

  if (DDS_RETCODE_OK == retcode)
  {
    RTIROS2_Graph_queue_update(self);
  }
  
done:
  RTIOsapiSemaphore_give(self->mutex_self);
  return retcode;
}

RTIROS2_GraphEndpointHandle
RTIROS2_Graph_register_local_subscription(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle,
  DDS_DataReader * const sub_reader)
{
  RTIROS2_GraphEndpointHandle result = RTIROS2_GraphEndpointHandle_INVALID;
  if (NULL == self || NULL == sub_reader)
  {
    return RTIROS2_GraphEndpointHandle_INVALID;
  }
  RTIOsapiSemaphore_take(self->mutex_self, RTI_NTP_TIME_INFINITE);
  result = RTIROS2_Graph_register_local_subscriptionEA(
    self, node_handle, sub_reader);
  if (RTIROS2_GraphEndpointHandle_INVALID != result)
  {
    RTIROS2_Graph_queue_update(self);
  }
  RTIOsapiSemaphore_give(self->mutex_self);
  return result;
}

RTIROS2_GraphEndpointHandle
RTIROS2_Graph_register_local_publisher(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle,
  DDS_DataWriter * const pub_writer)
{
  RTIROS2_GraphEndpointHandle result = RTIROS2_GraphEndpointHandle_INVALID;
  if (NULL == self || NULL == pub_writer)
  {
    return RTIROS2_GraphEndpointHandle_INVALID;
  }
  RTIOsapiSemaphore_take(self->mutex_self, RTI_NTP_TIME_INFINITE);
  result = RTIROS2_Graph_register_local_publisherEA(
    self, node_handle, pub_writer);
  if (RTIROS2_GraphEndpointHandle_INVALID != result)
  {
    RTIROS2_Graph_queue_update(self);
  }
  RTIOsapiSemaphore_give(self->mutex_self);
  return result;
}

RTIROS2_GraphEndpointHandle
RTIROS2_Graph_register_local_client(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle,
  DDS_DataReader * const client_reader,
  DDS_DataWriter * const client_writer)
{
  RTIROS2_GraphEndpointHandle result = RTIROS2_GraphEndpointHandle_INVALID;
  if (NULL == self || NULL == client_reader || NULL == client_writer)
  {
    return RTIROS2_GraphEndpointHandle_INVALID;
  }
  RTIOsapiSemaphore_take(self->mutex_self, RTI_NTP_TIME_INFINITE);
  result = RTIROS2_Graph_register_local_clientEA(
    self, node_handle, client_reader, client_writer);
  if (RTIROS2_GraphEndpointHandle_INVALID != result)
  {
    RTIROS2_Graph_queue_update(self);
  }
  RTIOsapiSemaphore_give(self->mutex_self);
  return result;
}

RTIROS2_GraphEndpointHandle
RTIROS2_Graph_register_local_service(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle,
  DDS_DataReader * const service_reader,
  DDS_DataWriter * const service_writer)
{
  RTIROS2_GraphEndpointHandle result = RTIROS2_GraphEndpointHandle_INVALID;
  if (NULL == self || NULL == service_reader || NULL == service_writer)
  {
    return RTIROS2_GraphEndpointHandle_INVALID;
  }
  RTIOsapiSemaphore_take(self->mutex_self, RTI_NTP_TIME_INFINITE);
  result = RTIROS2_Graph_register_local_serviceEA(
    self, node_handle, service_reader, service_writer);
  if (RTIROS2_GraphEndpointHandle_INVALID != result)
  {
    RTIROS2_Graph_queue_update(self);
  }
  RTIOsapiSemaphore_give(self->mutex_self);
  return result;
}

DDS_ReturnCode_t
RTIROS2_Graph_unregister_local_node(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle)
{
  DDS_ReturnCode_t retcode = DDS_RETCODE_ERROR;
  RTIROS2_GraphNode * node = NULL;

  if (NULL == self)
  {
    /* TODO(asorbini) Log error */
    retcode = DDS_RETCODE_BAD_PARAMETER;
    goto done;
  }

  RTIOsapiSemaphore_take(self->mutex_self, RTI_NTP_TIME_INFINITE);

  node = RTIROS2_Graph_lookup_local_node_by_handle(self, node_handle);
  if (NULL == node)
  {
    /* TODO(asorbini) Log error */
    retcode = DDS_RETCODE_BAD_PARAMETER;
    goto done;
  }

  if (DDS_RETCODE_OK != RTIROS2_Graph_finalize_node(self, node))
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
RTIROS2_Graph_unregister_local_subscription(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle,
  DDS_DataReader * const sub_reader)
{
  if (NULL == self || NULL == sub_reader)
  {
    /* TODO(asorbini) Log error */
    return DDS_RETCODE_BAD_PARAMETER;
  }
  return RTIROS2_Graph_unregister_local_endpoint(self, node_handle, sub_reader, NULL);
}

DDS_ReturnCode_t
RTIROS2_Graph_unregister_local_publisher(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle,
  DDS_DataWriter * const pub_writer)
{
  if (NULL == self || NULL == pub_writer)
  {
    /* TODO(asorbini) Log error */
    return DDS_RETCODE_BAD_PARAMETER;
  }
  return RTIROS2_Graph_unregister_local_endpoint(self, node_handle, NULL, pub_writer);
}

DDS_ReturnCode_t
RTIROS2_Graph_unregister_local_client(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle,
  DDS_DataReader * const client_reader,
  DDS_DataWriter * const client_writer)
{
  if (NULL == self || NULL == client_reader || NULL == client_writer)
  {
    /* TODO(asorbini) Log error */
    return DDS_RETCODE_BAD_PARAMETER;
  }
  return RTIROS2_Graph_unregister_local_endpoint(
    self, node_handle, client_reader, client_writer);
}

DDS_ReturnCode_t
RTIROS2_Graph_unregister_local_service(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle,
  DDS_DataReader * const service_reader,
  DDS_DataWriter * const service_writer)
{
  if (NULL == self || NULL == service_reader || NULL == service_writer)
  {
    /* TODO(asorbini) Log error */
    return DDS_RETCODE_BAD_PARAMETER;
  }
  return RTIROS2_Graph_unregister_local_endpoint(
    self, node_handle, service_reader, service_writer);
}


DDS_ReturnCode_t
RTIROS2_Graph_unregister_local_subscription_by_handle(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle,
  const RTIROS2_GraphEndpointHandle sub_handle)
{
  if (NULL == self)
  {
    /* TODO(asorbini) Log error */
    return DDS_RETCODE_BAD_PARAMETER;
  }
  return RTIROS2_Graph_unregister_local_endpoint_by_handle(
    self, node_handle, sub_handle);
}

DDS_ReturnCode_t
RTIROS2_Graph_unregister_local_publisher_by_handle(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle,
  const RTIROS2_GraphEndpointHandle pub_handle)
{
  if (NULL == self)
  {
    /* TODO(asorbini) Log error */
    return DDS_RETCODE_BAD_PARAMETER;
  }
  return RTIROS2_Graph_unregister_local_endpoint_by_handle(
    self, node_handle, pub_handle);
}

DDS_ReturnCode_t
RTIROS2_Graph_unregister_local_client_by_handle(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle,
  const RTIROS2_GraphEndpointHandle client_handle)
{
  if (NULL == self)
  {
    /* TODO(asorbini) Log error */
    return DDS_RETCODE_BAD_PARAMETER;
  }
  return RTIROS2_Graph_unregister_local_endpoint_by_handle(
    self, node_handle, client_handle);
}

DDS_ReturnCode_t
RTIROS2_Graph_unregister_local_service_by_handle(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle,
  const RTIROS2_GraphEndpointHandle service_handle)
{
  if (NULL == self)
  {
    /* TODO(asorbini) Log error */
    return DDS_RETCODE_BAD_PARAMETER;
  }
  return RTIROS2_Graph_unregister_local_endpoint_by_handle(
    self, node_handle, service_handle);
}

DDS_DomainParticipant*
RTIROS2_Graph_get_node_participant(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle)
{
  DDS_DomainParticipant * result = NULL;
  RTIROS2_GraphNode * node = NULL;
  if (NULL == self)
  {
    /* TODO(asorbini) Log error */
    return NULL;
  }
  RTIOsapiSemaphore_take(self->mutex_self, RTI_NTP_TIME_INFINITE);

  node = RTIROS2_Graph_lookup_local_node_by_handle(self, node_handle);
  if (NULL == node)
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  result = node->dds_participant;

done:
  RTIOsapiSemaphore_give(self->mutex_self);
  return result;
}

DDS_ReturnCode_t
RTIROS2_Graph_get_node_name(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle,
  const char ** const name_out,
  const char ** const namespace_out)
{
  DDS_ReturnCode_t retcode = DDS_RETCODE_ERROR;
  RTIROS2_GraphNode * node = NULL;
  if (NULL == self || (NULL == name_out && NULL == namespace_out))
  {
    /* TODO(asorbini) Log error */
    return DDS_RETCODE_BAD_PARAMETER;
  }
  RTIOsapiSemaphore_take(self->mutex_self, RTI_NTP_TIME_INFINITE);

  node = RTIROS2_Graph_lookup_local_node_by_handle(self, node_handle);
  if (NULL == node)
  {
    /* TODO(asorbini) Log error */
    retcode = DDS_RETCODE_PRECONDITION_NOT_MET;
    goto done;
  }

  if (NULL != name_out)
  {
    *name_out = node->node_name;
  }
  if (NULL != namespace_out)
  {
    *namespace_out = node->node_namespace;
  }

  retcode = DDS_RETCODE_OK;

done:
  RTIOsapiSemaphore_give(self->mutex_self);
  return retcode;
}

DDS_DataReader*
RTIROS2_Graph_get_endpoint_reader(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle,
  const RTIROS2_GraphEndpointHandle endp_handle)
{
  DDS_DataReader * result = NULL;
  RTIROS2_GraphNode * node = NULL;
  RTIROS2_GraphEndpoint * endp = NULL;
  if (NULL == self)
  {
    /* TODO(asorbini) Log error */
    return NULL;
  }
  RTIOsapiSemaphore_take(self->mutex_self, RTI_NTP_TIME_INFINITE);

  node = RTIROS2_Graph_lookup_local_node_by_handle(self, node_handle);
  if (NULL == node)
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  endp = RTIROS2_Graph_lookup_local_endpoint_by_handle(self, node, endp_handle);
  if (NULL == endp)
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  result = endp->dds_reader;

done:
  RTIOsapiSemaphore_give(self->mutex_self);
  return result;
}

DDS_DataWriter*
RTIROS2_Graph_get_endpoint_writer(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle,
  const RTIROS2_GraphEndpointHandle endp_handle)
{
  DDS_DataWriter * result = NULL;
  RTIROS2_GraphNode * node = NULL;
  RTIROS2_GraphEndpoint * endp = NULL;
  if (NULL == self)
  {
    /* TODO(asorbini) Log error */
    return NULL;
  }
  RTIOsapiSemaphore_take(self->mutex_self, RTI_NTP_TIME_INFINITE);

  node = RTIROS2_Graph_lookup_local_node_by_handle(self, node_handle);
  if (NULL == node)
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  endp = RTIROS2_Graph_lookup_local_endpoint_by_handle(self, node, endp_handle);
  if (NULL == endp)
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  result = endp->dds_writer;

done:
  RTIOsapiSemaphore_give(self->mutex_self);
  return result;
}

RTIROS2_GraphEndpointType_t
RTIROS2_Graph_get_endpoint_type(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle,
  const RTIROS2_GraphEndpointHandle endp_handle)
{
  RTIROS2_GraphEndpointType_t result = RTIROS2_GRAPH_ENDPOINT_UNKNOWN;
  RTIROS2_GraphNode * node = NULL;
  RTIROS2_GraphEndpoint * endp = NULL;
  if (NULL == self)
  {
    /* TODO(asorbini) Log error */
    return RTIROS2_GRAPH_ENDPOINT_UNKNOWN;
  }
  RTIOsapiSemaphore_take(self->mutex_self, RTI_NTP_TIME_INFINITE);

  node = RTIROS2_Graph_lookup_local_node_by_handle(self, node_handle);
  if (NULL == node)
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  endp = RTIROS2_Graph_lookup_local_endpoint_by_handle(self, node, endp_handle);
  if (NULL == endp)
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  result = endp->endp_type;

done:
  RTIOsapiSemaphore_give(self->mutex_self);
  return result;
}

DDS_ReturnCode_t
RTIROS2_Graph_compute_participant_gid(
  DDS_DomainParticipant * const dds_participant, RTIROS2_Gid * const gid)
{
  DDS_ReturnCode_t retcode = DDS_RETCODE_ERROR;
  DDS_InstanceHandle_t ih = DDS_HANDLE_NIL;

  if (NULL == dds_participant || NULL == gid)
  {
    retcode = DDS_RETCODE_BAD_PARAMETER;
    goto done;
  }

  ih = DDS_Entity_get_instance_handle(
    DDS_DomainParticipant_as_entity(dds_participant));

  RTIROS2_Graph_ih_to_gid(&ih, gid);

  retcode = DDS_RETCODE_OK;
  
done:
  return retcode;
}

DDS_ReturnCode_t
RTIROS2_Graph_compute_reader_gid(
  DDS_DataReader * const dds_reader, RTIROS2_Gid * const gid)
{
  DDS_ReturnCode_t retcode = DDS_RETCODE_ERROR;
  DDS_InstanceHandle_t ih = DDS_HANDLE_NIL;

  if (NULL == dds_reader || NULL == gid)
  {
    retcode = DDS_RETCODE_BAD_PARAMETER;
    goto done;
  }

  ih = DDS_Entity_get_instance_handle(DDS_DataReader_as_entity(dds_reader));

  RTIROS2_Graph_ih_to_gid(&ih, gid);

  retcode = DDS_RETCODE_OK;
  
done:
  return retcode;
}

DDS_ReturnCode_t
RTIROS2_Graph_compute_writer_gid(
  DDS_DataWriter * const dds_writer, RTIROS2_Gid * const gid)
{
  DDS_ReturnCode_t retcode = DDS_RETCODE_ERROR;
  DDS_InstanceHandle_t ih = DDS_HANDLE_NIL;

  if (NULL == dds_writer || NULL == gid)
  {
    retcode = DDS_RETCODE_BAD_PARAMETER;
    goto done;
  }

  ih = DDS_Entity_get_instance_handle(DDS_DataWriter_as_entity(dds_writer));

  RTIROS2_Graph_ih_to_gid(&ih, gid);

  retcode = DDS_RETCODE_OK;
  
done:
  return retcode;
}