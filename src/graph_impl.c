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

#include "limits.h"

#include "graph_impl.h"

#include "rmw_dds_common/msg/GraphSupport.h"

const char * const RTIROS2_GRAPH_THREAD_NAME = "ros2-graph";

DDS_ReturnCode_t
RTIROS2_Graph_get_graph_writer_qos(
  RTIROS2_Graph * const self,
  struct DDS_DataWriterQos * const writer_qos)
{
  DDS_ReturnCode_t retcode = DDS_RETCODE_ERROR;
  const char * graph_topic_name = NULL;

  graph_topic_name = DDS_TopicDescription_get_name(
    DDS_Topic_as_topicdescription(self->graph_topic));
  if (NULL == graph_topic_name)
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  if (DDS_RETCODE_OK !=
    DDS_Publisher_get_default_datawriter_qos_w_topic_name(self->graph_publisher,
      writer_qos, graph_topic_name))
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
  writer_qos->publish_mode.kind = DDS_ASYNCHRONOUS_PUBLISH_MODE_QOS;
  
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
  if (DDS_RETCODE_OK != retcode)
  {
      /* handle failure */
  }
  return retcode;
}


DDS_ReturnCode_t
RTIROS2_Graph_finalize_node(
  RTIROS2_Graph * const self,
  RTIROS2_GraphNode * const node)
{
  DDS_ReturnCode_t retcode = DDS_RETCODE_ERROR;
  RTIROS2_GraphEndpoint *endp =
    (RTIROS2_GraphEndpoint*) REDAInlineList_getFirst(&node->endpoints);

  REDAInlineList_removeNodeEA(&self->nodes, &node->list_node);
  self->nodes_len -= 1;

  while (NULL != endp)
  {
    RTIROS2_GraphEndpoint * const next_endp =
      (RTIROS2_GraphEndpoint*) REDAInlineListNode_getNext(&endp->list_node);

    if (DDS_RETCODE_OK != RTIROS2_Graph_finalize_endpoint(self, node, endp))
    {
      /* TODO(asorbini) Log error */
      goto done;
    }
    endp = next_endp;
  }

  if (NULL != node->node_name)
  {
    DDS_String_free(node->node_name);
  }

  if (NULL != node->node_namespace)
  {
    DDS_String_free(node->node_namespace);
  }

  REDAFastBufferPool_returnBuffer(self->nodes_pool, node);

  retcode = DDS_RETCODE_OK;
done:
  return retcode;
}

DDS_ReturnCode_t
RTIROS2_Graph_finalize_endpoint(
  RTIROS2_Graph * const self,
  RTIROS2_GraphNode * const node,
  RTIROS2_GraphEndpoint * const endp)
{
  REDAInlineList_removeNodeEA(&node->endpoints, &endp->list_node);
  node->endpoints_len -= 1;

  if (NULL != endp->dds_writer)
  {
    node->writers_len -= 1;
  }

  if (NULL != endp->dds_reader)
  {
    node->readers_len -= 1;
  }
  if (NULL != endp->endp_data)
  {
    RTIOsapiHeap_free(endp->endp_data);
  }
  REDAFastBufferPool_returnBuffer(self->endpoints_pool, endp);

  return DDS_RETCODE_OK;
}


RTIROS2_GraphNodeHandle
RTIROS2_Graph_next_node_handle(RTIROS2_Graph * const self)
{
  DDS_Boolean found = DDS_BOOLEAN_FALSE;
  RTIROS2_GraphNodeHandle candidate = RTIROS2_GraphNodeHandle_INVALID;
  RTIROS2_GraphNode * existing = NULL;
  DDS_Long try_count = 0;
  
  do {
    candidate = self->node_handle_next++;
    if (candidate < 0) {
      candidate = 0;
      self->node_handle_next = 0;
    }
    existing = RTIROS2_Graph_lookup_local_node_by_handle(self, candidate);
    found = NULL == existing;
    try_count += 1;
  } while (!found && try_count < INT_MAX);

  return (!found)? RTIROS2_GraphNodeHandle_INVALID : candidate;
}

RTIROS2_GraphNodeHandle
RTIROS2_Graph_next_endpoint_handle(
  RTIROS2_Graph * const self,
  RTIROS2_GraphNode * const node)
{
  DDS_Boolean found = DDS_BOOLEAN_FALSE;
  RTIROS2_GraphEndpointHandle candidate = RTIROS2_GraphEndpointHandle_INVALID;
  RTIROS2_GraphEndpoint * existing = NULL;
  DDS_Long try_count = 0;
  
  do {
    candidate = node->endp_handle_next++;
    if (candidate < 0) {
      candidate = 0;
      node->endp_handle_next = 0;
    }
    existing = RTIROS2_Graph_lookup_local_endpoint_by_handle(self, node, candidate);
    found = NULL == existing;
    try_count += 1;
  } while (!found && try_count < INT_MAX);

  return (!found)? RTIROS2_GraphEndpointHandle_INVALID : candidate;
}

RTIROS2_GraphNode*
RTIROS2_Graph_lookup_local_node_by_name(
  RTIROS2_Graph * const self,
  const char * const node_name,
  const char * const node_namespace)
{
  RTIROS2_GraphNode * node =
    (RTIROS2_GraphNode *)REDAInlineList_getFirst(&self->nodes);

  while (NULL != node)
  {
    if (strcmp(node->node_name, node_name) == 0 &&
      ((NULL == node->node_namespace && NULL == node_namespace) ||
      (NULL != node->node_namespace && NULL != node_namespace &&
        strcmp(node->node_namespace, node_namespace) == 0)))
    {
      return node;
    }
    node = (RTIROS2_GraphNode *) REDAInlineListNode_getNext(&node->list_node);
  }

  return NULL;
}

RTIROS2_GraphNode*
RTIROS2_Graph_lookup_local_node_by_handle(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle)
{
  RTIROS2_GraphNode * node =
    (RTIROS2_GraphNode *) REDAInlineList_getFirst(&self->nodes);

  while (NULL != node)
  {
    if (node->handle == node_handle)
    {
      return node;
    }
    node = (RTIROS2_GraphNode *) REDAInlineListNode_getNext(&node->list_node);
  }

  return NULL;
}

RTIROS2_GraphEndpoint*
RTIROS2_Graph_lookup_local_endpoint_by_handle(
  RTIROS2_Graph * const self,
  RTIROS2_GraphNode * const node,
  const RTIROS2_GraphEndpointHandle endp_handle)
{
  RTIROS2_GraphEndpoint * endp =
    (RTIROS2_GraphEndpoint *) REDAInlineList_getFirst(&node->endpoints);

  while (NULL != endp)
  {
    if (endp->handle == endp_handle)
    {
      return endp;
    }
    endp =
      (RTIROS2_GraphEndpoint *) REDAInlineListNode_getNext(&endp->list_node);
  }

  return NULL;
}

RTIROS2_GraphEndpoint*
RTIROS2_Graph_lookup_local_endpoint_by_dds_endpoints(
  RTIROS2_Graph * const self,
  RTIROS2_GraphNode * const node,
  DDS_DataReader * const dds_reader,
  DDS_DataWriter * const dds_writer)
{
  RTIROS2_GraphEndpoint * endp =
    (RTIROS2_GraphEndpoint *) REDAInlineList_getFirst(&node->endpoints);
  while (NULL != endp)
  {
    if ((NULL == dds_reader || endp->dds_reader == dds_reader) &&
      (NULL == dds_writer || endp->dds_writer == dds_writer))
    {
      return endp;
    }
    endp = (RTIROS2_GraphEndpoint *)
      REDAInlineListNode_getNext(&endp->list_node);
  }

  return NULL;
}

RTIROS2_GraphEndpoint*
RTIROS2_Graph_lookup_local_endpoint_by_topic_name(
  RTIROS2_Graph * const self,
  RTIROS2_GraphNode * const node,
  const char * const topic_name,
  const DDS_Boolean writer)
{
  RTIROS2_GraphEndpoint * endp =
    (RTIROS2_GraphEndpoint *) REDAInlineList_getFirst(&node->endpoints);
  const char * endp_topic_name = NULL;
  while (NULL != endp)
  {
    if (writer)
    {
      endp_topic_name = DDS_TopicDescription_get_name(
        DDS_Topic_as_topicdescription(
          DDS_DataWriter_get_topic(endp->dds_writer)));
    }
    else
    {
      endp_topic_name = DDS_TopicDescription_get_name(
        DDS_DataReader_get_topicdescription(endp->dds_reader));
    }
    if (strcmp(topic_name, endp_topic_name) == 0)
    {
      return endp;
    }
    endp = (RTIROS2_GraphEndpoint *)
      REDAInlineListNode_getNext(&endp->list_node);
  }

  return NULL;
}

RTIROS2_GraphEndpointType_t
RTIROS2_Graph_detect_topic_type(
  const char * const topic_name,
  const DDS_Boolean writer)
{
  if (strncmp(topic_name, "rt/", 3) == 0)
  {
    return (writer)?
      RTIROS2_GRAPH_ENDPOINT_PUBLISHER : RTIROS2_GRAPH_ENDPOINT_SUBSCRIPTION;
  }
  else if (strncmp(topic_name, "rr/", 3) == 0)
  {
    return (writer)?
      RTIROS2_GRAPH_ENDPOINT_SERVICE : RTIROS2_GRAPH_ENDPOINT_CLIENT;
  }
  else if (strncmp(topic_name, "rq/", 3) == 0)
  {
    return (writer)?
      RTIROS2_GRAPH_ENDPOINT_CLIENT : RTIROS2_GRAPH_ENDPOINT_SERVICE;
  }
  else
  {
    return RTIROS2_GRAPH_ENDPOINT_UNKNOWN;
  }
}


RTIROS2_GraphEndpointHandle
RTIROS2_Graph_register_local_subscriptionEA(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle,
  DDS_DataReader * const sub_reader)
{
  RTIROS2_GraphEndpointHandle result = RTIROS2_GraphEndpointHandle_INVALID;
  RTIROS2_GraphNode * node = NULL;
  RTIROS2_GraphEndpoint * endp = NULL;

  node = RTIROS2_Graph_lookup_local_node_by_handle(self, node_handle);
  if (NULL == node)
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  endp = RTIROS2_Graph_register_local_endpoint(
    self, node, RTIROS2_GRAPH_ENDPOINT_SUBSCRIPTION, sub_reader, NULL);
  if (NULL == endp)
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  RTIROS2_Graph_queue_update(self);

  result = endp->handle;
  
done:
  if (RTIROS2_GraphEndpointHandle_INVALID == result)
  {
    if (NULL != endp)
    {
      RTIROS2_Graph_finalize_endpoint(self, node, endp);
    }
  }
  return result;
}

RTIROS2_GraphEndpointHandle
RTIROS2_Graph_register_local_publicationEA(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle,
  DDS_DataWriter * const pub_writer)
{
  RTIROS2_GraphEndpointHandle result = RTIROS2_GraphEndpointHandle_INVALID;
  RTIROS2_GraphNode * node = NULL;
  RTIROS2_GraphEndpoint * endp = NULL;

  node = RTIROS2_Graph_lookup_local_node_by_handle(self, node_handle);
  if (NULL == node)
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  endp = RTIROS2_Graph_register_local_endpoint(
    self, node, RTIROS2_GRAPH_ENDPOINT_PUBLISHER, NULL, pub_writer);
  if (NULL == endp)
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  RTIROS2_Graph_queue_update(self);

  result = endp->handle;
  
done:
  if (RTIROS2_GraphEndpointHandle_INVALID == result)
  {
    if (NULL != endp)
    {
      RTIROS2_Graph_finalize_endpoint(self, node, endp);
    }
  }
  return result;
}

RTIROS2_GraphEndpointHandle
RTIROS2_Graph_register_local_clientEA(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle,
  DDS_DataReader * const client_reader,
  DDS_DataWriter * const client_writer)
{
  RTIROS2_GraphEndpointHandle result = RTIROS2_GraphEndpointHandle_INVALID;
  RTIROS2_GraphNode * node = NULL;
  RTIROS2_GraphEndpoint * endp = NULL;

  node = RTIROS2_Graph_lookup_local_node_by_handle(self, node_handle);
  if (NULL == node)
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  endp = RTIROS2_Graph_register_local_endpoint(
    self, node, RTIROS2_GRAPH_ENDPOINT_CLIENT, client_reader, client_writer);
  if (NULL == endp)
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  RTIROS2_Graph_queue_update(self);

  result = endp->handle;
  
done:
  if (RTIROS2_GraphEndpointHandle_INVALID == result)
  {
    if (NULL != endp)
    {
      RTIROS2_Graph_finalize_endpoint(self, node, endp);
    }
  }

  return result;
}

RTIROS2_GraphEndpointHandle
RTIROS2_Graph_register_local_serviceEA(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle,
  DDS_DataReader * const service_reader,
  DDS_DataWriter * const service_writer)
{
  RTIROS2_GraphEndpointHandle result = RTIROS2_GraphEndpointHandle_INVALID;
  RTIROS2_GraphNode * node = NULL;
  RTIROS2_GraphEndpoint * endp = NULL;

  node = RTIROS2_Graph_lookup_local_node_by_handle(self, node_handle);
  if (NULL == node)
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  endp = RTIROS2_Graph_register_local_endpoint(
    self, node, RTIROS2_GRAPH_ENDPOINT_SERVICE, service_reader, service_writer);
  if (NULL == endp)
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  RTIROS2_Graph_queue_update(self);

  result = endp->handle;
  
done:
  if (RTIROS2_GraphEndpointHandle_INVALID == result)
  {
    if (NULL != endp)
    {
      RTIROS2_Graph_finalize_endpoint(self, node, endp);
    }
  }
  return result;
}

RTIROS2_GraphEndpoint *
RTIROS2_Graph_register_local_endpoint(
  RTIROS2_Graph * const self,
  RTIROS2_GraphNode * const node,
  const RTIROS2_GraphEndpointType_t endp_type,
  DDS_DataReader * const dds_reader,
  DDS_DataWriter * const dds_writer)
{
  RTIROS2_GraphEndpoint * result = NULL;
  static const RTIROS2_GraphEndpoint def_endp = RTIROS2_GraphEndpoint_INITIALIZER;
  RTIROS2_GraphEndpointHandle endp_handle = RTIROS2_GraphEndpointHandle_INVALID;

  endp_handle = RTIROS2_Graph_next_endpoint_handle(self, node);
  if (RTIROS2_GraphEndpointHandle_INVALID == endp_handle)
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  result =
    (RTIROS2_GraphEndpoint*) REDAFastBufferPool_getBuffer(self->endpoints_pool);
  if (NULL == result)
  {
    /* TODO(asorbini) Log error */
    goto done;
  }
  *result = def_endp;
  
  REDAInlineList_addNodeToBackEA(&node->endpoints, &result->list_node);
  node->endpoints_len += 1;

  result->endp_type = endp_type;

  if (NULL != dds_reader)
  {
    result->dds_reader = dds_reader;
    node->writers_len += 1;
  }

  if (NULL != dds_writer)
  {
    result->dds_writer = dds_writer;
    node->writers_len += 1;
  }


done:
  return result;
}

DDS_ReturnCode_t
RTIROS2_Graph_unregister_local_endpoint(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle,
  DDS_DataReader * const endp_reader,
  DDS_DataWriter * const endp_writer)
{
  DDS_ReturnCode_t retcode = DDS_RETCODE_ERROR;
  RTIROS2_GraphNode * node = NULL;
  RTIROS2_GraphEndpoint * endp = NULL;

  RTIOsapiSemaphore_take(self->mutex_self, RTI_NTP_TIME_INFINITE);

  node = RTIROS2_Graph_lookup_local_node_by_handle(self, node_handle);
  if (NULL == node)
  {
    /* TODO(asorbini) Log error */
    retcode = DDS_RETCODE_BAD_PARAMETER;
    goto done;
  }

  endp = RTIROS2_Graph_lookup_local_endpoint_by_dds_endpoints(
    self, node, endp_reader, endp_writer);
  if (NULL == endp)
  {
    /* TODO(asorbini) Log error */
    retcode = DDS_RETCODE_BAD_PARAMETER;
    goto done;
  }

  if (DDS_RETCODE_OK != RTIROS2_Graph_finalize_endpoint(self, node, endp))
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  RTIROS2_Graph_queue_update(self);

  retcode = DDS_RETCODE_OK;
  
done:
  RTIOsapiSemaphore_give(self->mutex_self);
  return retcode;
}


DDS_ReturnCode_t
RTIROS2_Graph_unregister_local_endpoint_by_handle(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle,
  const RTIROS2_GraphEndpointHandle endp_handle)
{
  DDS_ReturnCode_t retcode = DDS_RETCODE_ERROR;
  RTIROS2_GraphNode * node = NULL;
  RTIROS2_GraphEndpoint * endp = NULL;

  RTIOsapiSemaphore_take(self->mutex_self, RTI_NTP_TIME_INFINITE);

  node = RTIROS2_Graph_lookup_local_node_by_handle(self, node_handle);
  if (NULL == node)
  {
    /* TODO(asorbini) Log error */
    retcode = DDS_RETCODE_BAD_PARAMETER;
    goto done;
  }

  endp = RTIROS2_Graph_lookup_local_endpoint_by_handle(self, node, endp_handle);
  if (NULL == endp)
  {
    /* TODO(asorbini) Log error */
    retcode = DDS_RETCODE_BAD_PARAMETER;
    goto done;
  }

  if (DDS_RETCODE_OK != RTIROS2_Graph_finalize_endpoint(self, node, endp))
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  RTIROS2_Graph_queue_update(self);

  retcode = DDS_RETCODE_OK;
  
done:
  RTIOsapiSemaphore_give(self->mutex_self);
  return retcode;
}
void
RTIROS2_Graph_ih_to_gid(
  const DDS_InstanceHandle_t * const ih,
  rmw_dds_common_msg_Gid * const gid)
{
  memset(gid, 0, sizeof(rmw_dds_common_msg_Gid));
  memcpy(gid->data, ih->keyHash.value, MIG_RTPS_KEY_HASH_MAX_LENGTH);
}

DDS_ReturnCode_t
RTIROS2_Graph_node_to_sample(
  RTIROS2_Graph * const self,
  RTIROS2_GraphNode * const node,
  rmw_dds_common_msg_NodeEntitiesInfo * const sample)
{
  DDS_ReturnCode_t retcode = DDS_RETCODE_ERROR;
  DDS_InstanceHandle_t ih = DDS_HANDLE_NIL;
  RTIROS2_GraphEndpoint * endp = NULL;
  DDS_Long reader_i = 0;
  DDS_Long writer_i = 0;
  DDS_Long endp_i = 0;
  struct rmw_dds_common_msg_GidSeq * gid_seq = NULL;
  rmw_dds_common_msg_Gid * gid = NULL;

  if (NULL != sample->node_name)
  {
    DDS_String_free(sample->node_name);
  }
  sample->node_name = DDS_String_dup(node->node_name);
  if (NULL == sample->node_name)
  {
    /* TODO(asorbini) Log error */
    goto done;
  }
  if (NULL != sample->node_namespace)
  {
    DDS_String_free(sample->node_namespace);
  }
  if (NULL != node->node_namespace)
  {
    sample->node_namespace = DDS_String_dup(node->node_namespace);
    if (NULL == sample->node_namespace)
    {
      /* TODO(asorbini) Log error */
      goto done;
    }
  }

  if (!rmw_dds_common_msg_GidSeq_ensure_length(
    &sample->reader_gid_seq, node->readers_len, node->readers_len))
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  if (!rmw_dds_common_msg_GidSeq_ensure_length(
    &sample->writer_gid_seq, node->writers_len, node->writers_len))
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  endp = (RTIROS2_GraphEndpoint*) REDAInlineList_getFirst(&node->endpoints);
  while (NULL != endp)
  {
    if (NULL != endp->dds_reader)
    {
      ih = DDS_Entity_get_instance_handle(
        DDS_DataReader_as_entity(endp->dds_reader));
      gid_seq = &sample->reader_gid_seq;
      endp_i = reader_i++;
    }
    if (NULL != endp->dds_writer)
    {
      ih = DDS_Entity_get_instance_handle(
        DDS_DataWriter_as_entity(endp->dds_writer));
      gid_seq = &sample->writer_gid_seq;
      endp_i = writer_i++;
    }
    gid = rmw_dds_common_msg_GidSeq_get_reference(gid_seq, endp_i);
    RTIROS2_Graph_ih_to_gid(&ih, gid);
    endp = (RTIROS2_GraphEndpoint*) REDAInlineListNode_getNext(&endp->list_node);
  }

  retcode = DDS_RETCODE_OK;
done:
  return retcode;
}

DDS_ReturnCode_t
RTIROS2_Graph_to_sample(
  RTIROS2_Graph * const self,
  rmw_dds_common_msg_ParticipantEntitiesInfo * const sample)
{
  DDS_ReturnCode_t retcode = DDS_RETCODE_ERROR;
  DDS_InstanceHandle_t ih = DDS_HANDLE_NIL;
  RTIROS2_GraphNode * node = NULL;
  rmw_dds_common_msg_NodeEntitiesInfo * nsample = NULL;
  DDS_Long node_i = 0;

  ih = DDS_Entity_get_instance_handle(
    DDS_DomainParticipant_as_entity(self->graph_participant));

  RTIROS2_Graph_ih_to_gid(&ih, &sample->gid);

  if (!rmw_dds_common_msg_NodeEntitiesInfoSeq_ensure_length(
    &sample->node_entities_info_seq, self->nodes_len, self->nodes_len))
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  node = (RTIROS2_GraphNode *) REDAInlineList_getFirst(&self->nodes);
  while (NULL != node)
  {
    nsample = rmw_dds_common_msg_NodeEntitiesInfoSeq_get_reference(
      &sample->node_entities_info_seq, node_i);
    if (DDS_RETCODE_OK != RTIROS2_Graph_node_to_sample(self, node, nsample))
    {
      /* TODO(asorbini) Log error */
      goto done;
    }
    node = (RTIROS2_GraphNode *) REDAInlineListNode_getNext(&node->list_node);
    node_i += 1;
  }
  retcode = DDS_RETCODE_OK;
done:
  return retcode;
}

DDS_ReturnCode_t
RTIROS2_Graph_publish_update(RTIROS2_Graph * const self)
{
  DDS_ReturnCode_t retcode = DDS_RETCODE_ERROR;
  rmw_dds_common_msg_ParticipantEntitiesInfoDataWriter * writer =
    rmw_dds_common_msg_ParticipantEntitiesInfoDataWriter_narrow(self->graph_writer);

  RTIOsapiSemaphore_take(self->mutex_self, RTI_NTP_TIME_INFINITE);

  if (DDS_RETCODE_OK != RTIROS2_Graph_to_sample(self, self->pinfo))
  {
    /* TODO(asorbini) Log error */
    RTIOsapiSemaphore_give(self->mutex_self);
    goto done;
  }

  RTIOsapiSemaphore_give(self->mutex_self);

  if (DDS_RETCODE_OK !=
    rmw_dds_common_msg_ParticipantEntitiesInfoDataWriter_write(
      writer, self->pinfo, &DDS_HANDLE_NIL))
  {
    /* TODO(asorbini) Log error */
    goto done;
  }
  retcode = DDS_RETCODE_OK;
  
done:
  return retcode;
}

void*
RTIROS2_Graph_update_thread(void * param)
{
  RTIROS2_Graph * const self = (RTIROS2_Graph *)param;
  RTIROS2_GraphNode * node = NULL;
  struct RTINtpTime ts_zero = RTI_NTP_TIME_ZERO;
  struct RTINtpTime ts_wait = RTI_NTP_TIME_ZERO;
  RTIOsapiSemaphoreStatus take_rc = RTI_OSAPI_SEMAPHORE_STATUS_ERROR;
  const DDS_Boolean enable_poll = RTIROS2_Graph_is_polling_enabled(self);

  RTINtpTime_packFromNanosec(ts_wait,
    self->poll_period.sec, self->poll_period.nanosec);

  while (self->thread_pinfo_active)
  {
    take_rc = RTIOsapiSemaphore_take(self->sem_pinfo,
      (enable_poll)?&ts_wait:RTI_NTP_TIME_INFINITE);

    if (!self->thread_pinfo_active)
    {
      continue;
    }

    if (enable_poll && RTI_OSAPI_SEMAPHORE_STATUS_TIMEOUT == take_rc)
    {
      RTIOsapiSemaphore_take(self->mutex_self, RTI_NTP_TIME_INFINITE);
      node = (RTIROS2_GraphNode*) REDAInlineList_getFirst(&self->nodes);
      while (NULL != node)
      {
        if (DDS_RETCODE_OK != RTIROS2_Graph_inspect_local_nodeEA(self, node))
        {
          /* TODO(asorbini) Log error */
        }
        node = (RTIROS2_GraphNode*)
          REDAInlineListNode_getNext(&node->list_node);
      }
      RTIOsapiSemaphore_give(self->mutex_self);
    }
    else if (RTI_OSAPI_SEMAPHORE_STATUS_OK == take_rc)
    {
      /* consume semaphore so we don't publish multiple updates unnecessarily */
      while (RTI_OSAPI_SEMAPHORE_STATUS_OK ==
        RTIOsapiSemaphore_take(self->sem_pinfo, &ts_zero)) {}

      if (DDS_RETCODE_OK != RTIROS2_Graph_publish_update(self))
      {
        /* TODO(asorbini) Log error */
      }
    }
  }

  RTIOsapiSemaphore_give(self->sem_pinfo_exit);
}

void
RTIROS2_Graph_queue_update(RTIROS2_Graph * const self)
{
  RTIOsapiSemaphore_give(self->sem_pinfo);
}

DDS_ReturnCode_t
RTIROS2_Graph_inspect_local_nodeEA(
  RTIROS2_Graph * const self,
  RTIROS2_GraphNode * const node)
{
  DDS_ReturnCode_t retcode = DDS_RETCODE_ERROR;
  struct DDS_PublisherSeq publishers_seq = DDS_SEQUENCE_INITIALIZER;
  struct DDS_SubscriberSeq subscribers_seq = DDS_SEQUENCE_INITIALIZER;
  struct DDS_DataWriterSeq writers_seq = DDS_SEQUENCE_INITIALIZER;
  struct DDS_DataReaderSeq readers_seq = DDS_SEQUENCE_INITIALIZER;
  DDS_Publisher * pub = NULL;
  DDS_Subscriber * sub = NULL;
  DDS_DataWriter * dw = NULL;
  DDS_DataReader * dr = NULL;
  DDS_Long seq_len = 0;
  DDS_Long nseq_len = 0;
  DDS_Long i = 0;
  DDS_Long j = 0;
  RTIROS2_GraphEndpointHandle endp_handle = RTIROS2_GraphEndpointHandle_INVALID;
  RTIROS2_GraphEndpointType_t endp_type = RTIROS2_GRAPH_ENDPOINT_UNKNOWN;
  const char * topic_name = NULL;
  RTIROS2_GraphEndpoint * endp = NULL;

  if (DDS_RETCODE_OK !=
    DDS_DomainParticipant_get_publishers(node->dds_participant, &publishers_seq))
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  seq_len = DDS_PublisherSeq_get_length(&publishers_seq);
  for (i = 0; i < seq_len; i++)
  {
    pub = *DDS_PublisherSeq_get_reference(&publishers_seq, i);

    if (DDS_RETCODE_OK != DDS_Publisher_get_all_datawriters(pub, &writers_seq))
    {
      /* TODO(asorbini) Log error */
      goto done;
    }
    nseq_len = DDS_DataWriterSeq_get_length(&writers_seq);
    for (j = 0; j < nseq_len; j++)
    {
      dw = *DDS_DataWriterSeq_get_reference(&writers_seq, j);
      endp = RTIROS2_Graph_lookup_local_endpoint_by_dds_endpoints(
        self, node, NULL, dw);
      if (NULL != endp)
      {
        continue;
      }
      topic_name = DDS_TopicDescription_get_name(
        DDS_Topic_as_topicdescription(
          DDS_DataWriter_get_topic(dw)));
      endp_type = RTIROS2_Graph_detect_topic_type(topic_name, DDS_BOOLEAN_TRUE);
      switch (endp_type)
      {
      case RTIROS2_GRAPH_ENDPOINT_PUBLISHER:
      {
        endp_handle =
          RTIROS2_Graph_register_local_publisherEA(self, node->handle, dw);
        break;
      }
      case RTIROS2_GRAPH_ENDPOINT_CLIENT:
      {
        endp = RTIROS2_Graph_lookup_local_endpoint_by_topic_name(
          self, node, topic_name, DDS_BOOLEAN_FALSE);
        if (endp == NULL)
        {
          endp_handle =
            RTIROS2_Graph_register_local_clientEA(
              self, node->handle, NULL, dw);
        }
        else if (NULL != endp->dds_writer)
        {
          /* TODO(asorbini) Log error */
          goto done;
        }
        else
        {
          endp->dds_writer = dw;
        }
        break;
      }
      case RTIROS2_GRAPH_ENDPOINT_SERVICE:
      {
        endp = RTIROS2_Graph_lookup_local_endpoint_by_topic_name(
          self, node, topic_name, DDS_BOOLEAN_FALSE);
        if (endp == NULL)
        {
          endp_handle =
            RTIROS2_Graph_register_local_serviceEA(
              self, node->handle, NULL, dw);
        }
        else if (NULL != endp->dds_writer)
        {
          /* TODO(asorbini) Log error */
          goto done;
        }
        else
        {
          endp->dds_writer = dw;
        }
        break;
      }
      default:
      {
        continue;
      }
      }

      if (RTIROS2_GraphEndpointHandle_INVALID == endp_handle)
      {
        /* TODO(asorbini) Log error */
        goto done;
      }
    }
  }

  if (DDS_RETCODE_OK !=
    DDS_DomainParticipant_get_subscribers(
      node->dds_participant, &subscribers_seq))
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  seq_len = DDS_SubscriberSeq_get_length(&subscribers_seq);
  for (i = 0; i < seq_len; i++)
  {
    sub = *DDS_SubscriberSeq_get_reference(&subscribers_seq, i);

    if (DDS_RETCODE_OK != DDS_Subscriber_get_all_datareaders(sub, &readers_seq))
    {
      /* TODO(asorbini) Log error */
      goto done;
    }
    nseq_len = DDS_DataReaderSeq_get_length(&readers_seq);
    for (j = 0; j < nseq_len; j++)
    {
      dr = *DDS_DataReaderSeq_get_reference(&readers_seq, j);
      endp = RTIROS2_Graph_lookup_local_endpoint_by_dds_endpoints(
        self, node, NULL, dw);
      if (NULL != endp)
      {
        continue;
      }
      topic_name = DDS_TopicDescription_get_name(
        DDS_DataReader_get_topicdescription(dr));
      endp_type = RTIROS2_Graph_detect_topic_type(topic_name, DDS_BOOLEAN_FALSE);
      switch (endp_type)
      {
      case RTIROS2_GRAPH_ENDPOINT_SUBSCRIPTION:
      {
        endp_handle =
          RTIROS2_Graph_register_local_subscriptionEA(self, node->handle, dr);
        break;
      }
      case RTIROS2_GRAPH_ENDPOINT_CLIENT:
      {
        endp = RTIROS2_Graph_lookup_local_endpoint_by_topic_name(
          self, node, topic_name, DDS_BOOLEAN_FALSE);
        if (NULL == endp)
        {
          endp_handle =
            RTIROS2_Graph_register_local_clientEA(self, node->handle, dr, NULL);
        }
        else if (NULL != endp->dds_reader)
        {
          /* TODO(asorbini) Log error */
          goto done;
        }
        else
        {
          endp->dds_reader = dr;
        }

        break;
      }
      case RTIROS2_GRAPH_ENDPOINT_SERVICE:
      {
        endp = RTIROS2_Graph_lookup_local_endpoint_by_topic_name(
          self, node, topic_name, DDS_BOOLEAN_FALSE);
        if (NULL == endp)
        {
          endp_handle =
            RTIROS2_Graph_register_local_serviceEA(self, node->handle, dr, NULL);
        }
        else if (NULL != endp->dds_reader)
        {
          /* TODO(asorbini) Log error */
          goto done;
        }
        else
        {
          endp->dds_reader = dr;
        }
        break;
      }
      default:
      {
        continue;
      }
      }

      if (RTIROS2_GraphEndpointHandle_INVALID == endp_handle)
      {
        /* TODO(asorbini) Log error */
        goto done;
      }
    }
  }
  

  retcode = DDS_RETCODE_OK;
  
done:
  DDS_PublisherSeq_finalize(&publishers_seq);
  DDS_SubscriberSeq_finalize(&subscribers_seq);
  DDS_DataWriterSeq_finalize(&writers_seq);
  DDS_DataReaderSeq_finalize(&readers_seq);
  return retcode;
}