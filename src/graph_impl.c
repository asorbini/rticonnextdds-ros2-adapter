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

DDS_ReturnCode_t
RTI_Ros2Graph_finalize_node(
  RTI_Ros2Graph * const self,
  RTI_Ros2GraphNode * const node)
{
  DDS_ReturnCode_t retcode = DDS_RETCODE_ERROR;
  RTI_Ros2GraphEndpoint *endp =
    (RTI_Ros2GraphEndpoint*) REDAInlineList_getFirst(&node->endpoints);

  REDAInlineList_removeNodeEA(&self->nodes, &node->list_node);
  self->nodes_len -= 1;

  while (NULL != endp)
  {
    RTI_Ros2GraphEndpoint * const next_endp =
      (RTI_Ros2GraphEndpoint*) REDAInlineListNode_getNext(&endp->list_node);

    if (DDS_RETCODE_OK != RTI_Ros2Graph_finalize_endpoint(self, node, endp))
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
RTI_Ros2Graph_finalize_endpoint(
  RTI_Ros2Graph * const self,
  RTI_Ros2GraphNode * const node,
  RTI_Ros2GraphEndpoint * const endp)
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


RTI_Ros2GraphNodeHandle
RTI_Ros2Graph_next_node_handle(RTI_Ros2Graph * const self)
{
  DDS_Boolean found = DDS_BOOLEAN_FALSE;
  RTI_Ros2GraphNodeHandle candidate = RTI_Ros2GraphNodeHandle_INVALID;
  RTI_Ros2GraphNode * existing = NULL;
  DDS_Long try_count = 0;
  
  do {
    candidate = self->node_handle_next++;
    if (candidate < 0) {
      candidate = 0;
      self->node_handle_next = 0;
    }
    existing = RTI_Ros2Graph_lookup_local_node_by_handle(self, candidate);
    found = NULL == existing;
    try_count += 1;
  } while (!found && try_count < INT_MAX);

  return (!found)? RTI_Ros2GraphNodeHandle_INVALID : candidate;
}

RTI_Ros2GraphNodeHandle
RTI_Ros2Graph_next_endpoint_handle(
  RTI_Ros2Graph * const self,
  RTI_Ros2GraphNode * const node)
{
  DDS_Boolean found = DDS_BOOLEAN_FALSE;
  RTI_Ros2GraphEndpointHandle candidate = RTI_Ros2GraphEndpointHandle_INVALID;
  RTI_Ros2GraphEndpoint * existing = NULL;
  DDS_Long try_count = 0;
  
  do {
    candidate = node->endp_handle_next++;
    if (candidate < 0) {
      candidate = 0;
      node->endp_handle_next = 0;
    }
    existing = RTI_Ros2Graph_lookup_local_endpoint_by_handle(self, node, candidate);
    found = NULL == existing;
    try_count += 1;
  } while (!found && try_count < INT_MAX);

  return (!found)? RTI_Ros2GraphEndpointHandle_INVALID : candidate;
}

RTI_Ros2GraphNode*
RTI_Ros2Graph_lookup_local_node_by_name(
  RTI_Ros2Graph * const self,
  const char * const node_name,
  const char * const node_namespace)
{
  RTI_Ros2GraphNode * node =
    (RTI_Ros2GraphNode *)REDAInlineList_getFirst(&self->nodes);

  while (NULL != node)
  {
    if (strcmp(node->node_name, node_name) == 0 &&
      ((NULL == node->node_namespace && NULL == node_namespace) ||
      (NULL != node->node_namespace && NULL != node_namespace &&
        strcmp(node->node_namespace, node_namespace) == 0)))
    {
      return node;
    }
    node = (RTI_Ros2GraphNode *) REDAInlineListNode_getNext(&node->list_node);
  }

  return NULL;
}

RTI_Ros2GraphNode*
RTI_Ros2Graph_lookup_local_node_by_handle(
  RTI_Ros2Graph * const self,
  const RTI_Ros2GraphNodeHandle node_handle)
{
  RTI_Ros2GraphNode * node =
    (RTI_Ros2GraphNode *) REDAInlineList_getFirst(&self->nodes);

  while (NULL != node)
  {
    if (node->handle == node_handle)
    {
      return node;
    }
    node = (RTI_Ros2GraphNode *) REDAInlineListNode_getNext(&node->list_node);
  }

  return NULL;
}

RTI_Ros2GraphEndpoint*
RTI_Ros2Graph_lookup_local_endpoint_by_handle(
  RTI_Ros2Graph * const self,
  RTI_Ros2GraphNode * const node,
  const RTI_Ros2GraphEndpointHandle endp_handle)
{
  RTI_Ros2GraphEndpoint * endp =
    (RTI_Ros2GraphEndpoint *) REDAInlineList_getFirst(&node->endpoints);

  while (NULL != endp)
  {
    if (endp->handle == endp_handle)
    {
      return endp;
    }
    endp =
      (RTI_Ros2GraphEndpoint *) REDAInlineListNode_getNext(&endp->list_node);
  }

  return NULL;
}

RTI_Ros2GraphEndpoint*
RTI_Ros2Graph_lookup_local_endpoint_by_dds_endpoints(
  RTI_Ros2Graph * const self,
  RTI_Ros2GraphNode * const node,
  DDS_DataReader * const dds_reader,
  DDS_DataWriter * const dds_writer)
{
  RTI_Ros2GraphEndpoint * endp =
    (RTI_Ros2GraphEndpoint *) REDAInlineList_getFirst(&node->endpoints);
  while (NULL != endp)
  {
    if (endp->dds_reader == dds_reader && endp->dds_writer == dds_writer)
    {
      return endp;
    }
    endp = (RTI_Ros2GraphEndpoint *)
      REDAInlineListNode_getNext(&endp->list_node);
  }

  return NULL;
}


RTI_Ros2GraphEndpoint *
RTI_Ros2Graph_register_local_endpoint(
  RTI_Ros2Graph * const self,
  RTI_Ros2GraphNode * const node,
  const RTI_Ros2GraphEndpointType_t endp_type,
  DDS_DataReader * const dds_reader,
  DDS_DataWriter * const dds_writer)
{
  RTI_Ros2GraphEndpoint * result = NULL;
  static const RTI_Ros2GraphEndpoint def_endp = RTI_Ros2GraphEndpoint_INITIALIZER;
  RTI_Ros2GraphEndpointHandle endp_handle = RTI_Ros2GraphEndpointHandle_INVALID;

  endp_handle = RTI_Ros2Graph_next_endpoint_handle(self, node);
  if (RTI_Ros2GraphEndpointHandle_INVALID == endp_handle)
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  result =
    (RTI_Ros2GraphEndpoint*) REDAFastBufferPool_getBuffer(self->endpoints_pool);
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
RTI_Ros2Graph_unregister_local_endpoint(
  RTI_Ros2Graph * const self,
  const RTI_Ros2GraphNodeHandle node_handle,
  DDS_DataReader * const endp_reader,
  DDS_DataWriter * const endp_writer)
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

  endp = RTI_Ros2Graph_lookup_local_endpoint_by_dds_endpoints(
    self, node, endp_reader, endp_writer);
  if (NULL == endp)
  {
    /* TODO(asorbini) Log error */
    retcode = DDS_RETCODE_BAD_PARAMETER;
    goto done;
  }

  if (DDS_RETCODE_OK != RTI_Ros2Graph_finalize_endpoint(self, node, endp))
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  RTI_Ros2Graph_queue_update(self);

  retcode = DDS_RETCODE_OK;
  
done:
  RTIOsapiSemaphore_give(self->mutex_self);
  return retcode;
}


DDS_ReturnCode_t
RTI_Ros2Graph_unregister_local_endpoint_by_handle(
  RTI_Ros2Graph * const self,
  const RTI_Ros2GraphNodeHandle node_handle,
  const RTI_Ros2GraphEndpointHandle endp_handle)
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

  endp = RTI_Ros2Graph_lookup_local_endpoint_by_handle(self, node, endp_handle);
  if (NULL == endp)
  {
    /* TODO(asorbini) Log error */
    retcode = DDS_RETCODE_BAD_PARAMETER;
    goto done;
  }

  if (DDS_RETCODE_OK != RTI_Ros2Graph_finalize_endpoint(self, node, endp))
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  RTI_Ros2Graph_queue_update(self);

  retcode = DDS_RETCODE_OK;
  
done:
  RTIOsapiSemaphore_give(self->mutex_self);
  return retcode;
}
void
RTI_Ros2Graph_ih_to_gid(
  const DDS_InstanceHandle_t * const ih,
  rmw_dds_common_msg_Gid * const gid)
{
  memset(gid, 0, sizeof(rmw_dds_common_msg_Gid));
  memcpy(gid->data, ih->keyHash.value, MIG_RTPS_KEY_HASH_MAX_LENGTH);
}

DDS_ReturnCode_t
RTI_Ros2Graph_node_to_sample(
  RTI_Ros2Graph * const self,
  RTI_Ros2GraphNode * const node,
  rmw_dds_common_msg_NodeEntitiesInfo * const sample)
{
  DDS_ReturnCode_t retcode = DDS_RETCODE_ERROR;
  DDS_InstanceHandle_t ih = DDS_HANDLE_NIL;
  RTI_Ros2GraphEndpoint * endp = NULL;
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

  endp = (RTI_Ros2GraphEndpoint*) REDAInlineList_getFirst(&node->endpoints);
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
    RTI_Ros2Graph_ih_to_gid(&ih, gid);
    endp = (RTI_Ros2GraphEndpoint*) REDAInlineListNode_getNext(&endp->list_node);
  }

  retcode = DDS_RETCODE_OK;
done:
  return retcode;
}

DDS_ReturnCode_t
RTI_Ros2Graph_to_sample(
  RTI_Ros2Graph * const self,
  rmw_dds_common_msg_ParticipantEntitiesInfo * const sample)
{
  DDS_ReturnCode_t retcode = DDS_RETCODE_ERROR;
  DDS_InstanceHandle_t ih = DDS_HANDLE_NIL;
  RTI_Ros2GraphNode * node = NULL;
  rmw_dds_common_msg_NodeEntitiesInfo * nsample = NULL;
  DDS_Long node_i = 0;

  ih = DDS_Entity_get_instance_handle(
    DDS_DomainParticipant_as_entity(self->dds_participant));

  RTI_Ros2Graph_ih_to_gid(&ih, &sample->gid);

  if (!rmw_dds_common_msg_NodeEntitiesInfoSeq_ensure_length(
    &sample->node_entities_info_seq, self->nodes_len, self->nodes_len))
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  node = (RTI_Ros2GraphNode *) REDAInlineList_getFirst(&self->nodes);
  while (NULL != node)
  {
    nsample = rmw_dds_common_msg_NodeEntitiesInfoSeq_get_reference(
      &sample->node_entities_info_seq, node_i);
    if (DDS_RETCODE_OK != RTI_Ros2Graph_node_to_sample(self, node, nsample))
    {
      /* TODO(asorbini) Log error */
      goto done;
    }
    node = (RTI_Ros2GraphNode *) REDAInlineListNode_getNext(&node->list_node);
    node_i += 1;
  }
  retcode = DDS_RETCODE_OK;
done:
  return retcode;
}

DDS_ReturnCode_t
RTI_Ros2Graph_publish_update(RTI_Ros2Graph * const self)
{
  DDS_ReturnCode_t retcode = DDS_RETCODE_ERROR;
  rmw_dds_common_msg_ParticipantEntitiesInfoDataWriter * writer =
    rmw_dds_common_msg_ParticipantEntitiesInfoDataWriter_narrow(self->graph_writer);

  RTIOsapiSemaphore_take(self->mutex_self, RTI_NTP_TIME_INFINITE);

  if (DDS_RETCODE_OK != RTI_Ros2Graph_to_sample(self, self->pinfo))
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
RTI_Ros2Graph_update_thread(void * param)
{
  RTI_Ros2Graph * const self = (RTI_Ros2Graph *)param;
  struct RTINtpTime ts_zero = RTI_NTP_TIME_ZERO;

  while (self->thread_pinfo_active)
  {
    RTIOsapiSemaphore_take(self->sem_pinfo, RTI_NTP_TIME_INFINITE);

    if (!self->thread_pinfo_active)
    {
      continue;
    }

    /* consume semaphore so we don't publish multiple updates unnecessarily */
    while (RTI_OSAPI_SEMAPHORE_STATUS_OK ==
      RTIOsapiSemaphore_take(self->sem_pinfo, &ts_zero)) {}

    if (DDS_RETCODE_OK != RTI_Ros2Graph_publish_update(self))
    {
      /* TODO(asorbini) Log error */
    }
  }

  RTIOsapiSemaphore_give(self->sem_pinfo_exit);
}

void
RTI_Ros2Graph_queue_update(RTI_Ros2Graph * const self)
{
  RTIOsapiSemaphore_give(self->sem_pinfo);
}