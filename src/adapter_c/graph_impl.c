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

const char * const RTIROS2_GRAPH_THREAD_NAME = "ros2-graph";


DDS_ReturnCode_t
RTIROS2_Graph_initialize(
  RTIROS2_Graph * const self,
  const struct RTIROS2_GraphProperties * const properties)
{
  DDS_ReturnCode_t retcode = DDS_RETCODE_ERROR;
  struct REDAFastBufferPoolProperty pool_props =
        REDA_FAST_BUFFER_POOL_PROPERTY_DEFAULT;
  static const RTIROS2_Graph def_self = RTIROS2_Graph_INITIALIZER;

  struct DDS_DataWriterQos graph_writer_qos = DDS_DataWriterQos_INITIALIZER;

  if (NULL == properties->graph_participant)
  {
    /* TODO(asorbini) Log error */
    retcode = DDS_RETCODE_BAD_PARAMETER;
    goto done;
  }

  *self = def_self;

  if (!RTIROS2_GraphSampleAdapter_is_valid(&properties->sample_adapter))
  {
    self->sample_adapter.convert_to_sample =
      RTIROS2_CGraphSampleAdapter_convert_to_sample;
    self->sample_adapter.publish_sample =
      RTIROS2_CGraphSampleAdapter_publish_sample;
    self->sample_adapter.alloc_sample =
      RTIROS2_CGraphSampleAdapter_alloc_sample;
    self->sample_adapter.free_sample =
      RTIROS2_CGraphSampleAdapter_free_sample;
  }
  else
  {
    self->sample_adapter = properties->sample_adapter;
  }

  self->nodes_pool =
    REDAFastBufferPool_newForStructure(RTIROS2_GraphNode, &pool_props);
  if (self->nodes_pool == NULL)
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  self->endpoints_pool =
    REDAFastBufferPool_newForStructure(RTIROS2_GraphEndpoint, &pool_props);
  if (self->endpoints_pool == NULL)
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  self->graph_participant = properties->graph_participant;

  if (NULL != properties->graph_publisher)
  {
    self->graph_publisher = properties->graph_publisher;
  }
  else
  {
    self->graph_publisher = DDS_DomainParticipant_create_publisher(
      self->graph_participant,
      &DDS_PUBLISHER_QOS_DEFAULT,
      NULL,
      DDS_STATUS_MASK_NONE);
    if (NULL == self->graph_publisher)
    {
      /* TODO(asorbini) Log error */
      goto done;
    }
    self->graph_publisher_own = DDS_BOOLEAN_TRUE;
  }

  if (NULL != properties->graph_topic)
  {
    self->graph_topic = properties->graph_topic;
  }
  else
  {
    if (DDS_RETCODE_OK !=
      RTIROS2_ParticipantEntitiesInfoTypeSupport_register_type(
        self->graph_participant, RTIROS2_GRAPH_TYPE_NAME))
    {
      /* TODO(asorbini) Log error */
      goto done;
    }

    self->graph_topic = DDS_DomainParticipant_find_topic(
      self->graph_participant,
      RTIROS2_GRAPH_TOPIC_NAME,
      &DDS_DURATION_ZERO);
    
    if (NULL == self->graph_topic)
    {
      self->graph_topic = DDS_DomainParticipant_create_topic(
        self->graph_participant,
        RTIROS2_GRAPH_TOPIC_NAME,
        RTIROS2_GRAPH_TYPE_NAME,
        &DDS_TOPIC_QOS_DEFAULT,
        NULL,
        DDS_STATUS_MASK_NONE);
      if (NULL == self->graph_topic)
      {
        /* TODO(asorbini) Log error */
        goto done;
      }
    }

    self->graph_topic_own = DDS_BOOLEAN_TRUE;
  }


  if (NULL != properties->graph_writer)
  {
    self->graph_writer = properties->graph_writer;
  }
  else
  {
    if (DDS_RETCODE_OK !=
      RTIROS2_Graph_get_graph_writer_qos(self, &graph_writer_qos))
    {
      /* TODO(asorbini) Log error */
      goto done;
    }

    self->graph_writer = DDS_Publisher_create_datawriter(self->graph_publisher,
      self->graph_topic,
      &graph_writer_qos,
      NULL,
      DDS_STATUS_MASK_NONE);
    if (NULL == self->graph_writer)
    {
      /* TODO(asorbini) Log error */
      goto done;
    }

    self->graph_writer_own = DDS_BOOLEAN_TRUE;
  }

  self->pinfo = RTIROS2_Graph_alloc_sample(self);
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

  self->poll_period = properties->poll_period;
  self->thread_pinfo_active = DDS_BOOLEAN_TRUE;
  self->thread_pinfo =
    RTIOsapiThread_new(
      RTIROS2_GRAPH_THREAD_NAME,
      RTI_OSAPI_THREAD_PRIORITY_DEFAULT,
      RTI_OSAPI_THREAD_OPTION_DEFAULT,
      RTI_OSAPI_THREAD_STACK_SIZE_DEFAULT,
      NULL,
      RTIROS2_Graph_update_thread,
      self);

  retcode = DDS_RETCODE_OK;
  
done:
  if (DDS_RETCODE_OK != retcode)
  {
    if (NULL != self)
    {
      RTIROS2_Graph_finalize(self);
    }
  }
  DDS_DataWriterQos_finalize(&graph_writer_qos);
  return retcode;
}

DDS_ReturnCode_t
RTIROS2_Graph_finalize(RTIROS2_Graph * const self)
{
  DDS_ReturnCode_t retcode = DDS_RETCODE_ERROR;
  RTIROS2_GraphNode * node = NULL;

  if (NULL != self->thread_pinfo)
  {
    self->thread_pinfo_active = DDS_BOOLEAN_FALSE;
    RTIOsapiSemaphore_give(self->sem_pinfo);
    RTIOsapiSemaphore_take(self->sem_pinfo_exit, RTI_NTP_TIME_INFINITE);
    RTIOsapiThread_delete(self->thread_pinfo);
  }
  node = (RTIROS2_GraphNode*) REDAInlineList_getFirst(&self->nodes);
  while (NULL != node)
  {
    RTIROS2_GraphNode * const next_node =
      (RTIROS2_GraphNode*) REDAInlineListNode_getNext(&node->list_node);

    if (DDS_RETCODE_OK != RTIROS2_Graph_finalize_node(self, node))
    {
      /* TODO(asorbini) Log error */
      goto done;
    }
    node = next_node;
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
    RTIROS2_Graph_free_sample(self, self->pinfo);
  }

  if (NULL != self->graph_writer && self->graph_writer_own)
  {
    if (DDS_RETCODE_OK !=
      DDS_Publisher_delete_datawriter(self->graph_publisher, self->graph_writer))
    {
      /* TODO(asorbini) Log error */
      goto done;
    }
  }

  if (NULL != self->graph_topic && self->graph_topic_own)
  {
    if (DDS_RETCODE_OK !=
      DDS_DomainParticipant_delete_topic(
        self->graph_participant, self->graph_topic))
    {
      /* TODO(asorbini) Log error */
      goto done;
    }
  }

  if (NULL != self->graph_publisher && self->graph_publisher_own)
  {
    if (DDS_RETCODE_OK !=
      DDS_DomainParticipant_delete_publisher(
        self->graph_participant, self->graph_publisher))
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

  if (DDS_RETCODE_OK != RTIROS2_Graph_customize_datawriter_qos(writer_qos))
  {
    /* TODO(asorbini) Log error */
    goto done;
  }
  
  retcode = DDS_RETCODE_OK;
  
done:
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

  UNUSED_ARG(self);

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

  UNUSED_ARG(self);

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
  UNUSED_ARG(self);
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
RTIROS2_Graph_register_local_publisherEA(
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
  RTIROS2_GraphEndpoint * existing = NULL;
  static const RTIROS2_GraphEndpoint def_endp = RTIROS2_GraphEndpoint_INITIALIZER;
  RTIROS2_GraphEndpointHandle endp_handle = RTIROS2_GraphEndpointHandle_INVALID;

  /* Check that the DDS endpoints have not already been assigned to another
    endpoint */
  existing = RTIROS2_Graph_lookup_local_endpoint_by_dds_endpoints(
    self, node, dds_reader, dds_writer);
  if (NULL != existing)
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

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

  result->handle = endp_handle;
  
  REDAInlineList_addNodeToBackEA(&node->endpoints, &result->list_node);
  node->endpoints_len += 1;

  result->endp_type = endp_type;

  if (NULL != dds_reader)
  {
    result->dds_reader = dds_reader;
    node->readers_len += 1;
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
  RTIROS2_Gid * const gid)
{
  memset(gid, 0, sizeof(RTIROS2_Gid));
  memcpy(gid->data, ih->keyHash.value, MIG_RTPS_KEY_HASH_MAX_LENGTH);
}

DDS_ReturnCode_t
RTIROS2_Graph_node_to_sample(
  RTIROS2_Graph * const self,
  RTIROS2_GraphNode * const node,
  RTIROS2_NodeEntitiesInfo * const sample)
{
  DDS_ReturnCode_t retcode = DDS_RETCODE_ERROR;
  DDS_InstanceHandle_t ih = DDS_HANDLE_NIL;
  RTIROS2_GraphEndpoint * endp = NULL;
  DDS_Long reader_i = 0;
  DDS_Long writer_i = 0;
  DDS_Long endp_i = 0;
  struct RTIROS2_GidSeq * gid_seq = NULL;
  RTIROS2_Gid * gid = NULL;

  UNUSED_ARG(self);

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
  if (NULL != node->node_namespace)
  {
    if (NULL != sample->node_namespace)
    {
      DDS_String_free(sample->node_namespace);
    }
    sample->node_namespace = DDS_String_dup(node->node_namespace);
    if (NULL == sample->node_namespace)
    {
      /* TODO(asorbini) Log error */
      goto done;
    }
  }
  else
  {
    if (sample->node_namespace[0] != '\0')
    {
      DDS_String_free(sample->node_namespace);
      sample->node_namespace = DDS_String_dup("");
      if (NULL == sample->node_namespace)
      {
        /* TODO(asorbini) Log error */
        goto done;
      }
    }
  }

  if (!RTIROS2_GidSeq_ensure_length(
    &sample->reader_gid_seq, node->readers_len, node->readers_len))
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  if (!RTIROS2_GidSeq_ensure_length(
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
    gid = RTIROS2_GidSeq_get_reference(gid_seq, endp_i);
    RTIROS2_Graph_ih_to_gid(&ih, gid);
    endp = (RTIROS2_GraphEndpoint*) REDAInlineListNode_getNext(&endp->list_node);
  }

  retcode = DDS_RETCODE_OK;
done:
  return retcode;
}

DDS_ReturnCode_t
RTIROS2_CGraphSampleAdapter_convert_to_sample(
  RTIROS2_Graph * const self,
  void * const vsample,
  void * const adapter_data)
{
  DDS_ReturnCode_t retcode = DDS_RETCODE_ERROR;
  DDS_InstanceHandle_t ih = DDS_HANDLE_NIL;
  RTIROS2_GraphNode * node = NULL;
  RTIROS2_NodeEntitiesInfo * nsample = NULL;
  DDS_Long node_i = 0;
  RTIROS2_ParticipantEntitiesInfo * sample =
    (RTIROS2_ParticipantEntitiesInfo*)vsample;

  UNUSED_ARG(adapter_data);

  ih = DDS_Entity_get_instance_handle(
    DDS_DomainParticipant_as_entity(self->graph_participant));

  RTIROS2_Graph_ih_to_gid(&ih, &sample->gid);

  if (!RTIROS2_NodeEntitiesInfoSeq_ensure_length(
    &sample->node_entities_info_seq, self->nodes_len, self->nodes_len))
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  node = (RTIROS2_GraphNode *) REDAInlineList_getFirst(&self->nodes);
  while (NULL != node)
  {
    nsample = RTIROS2_NodeEntitiesInfoSeq_get_reference(
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

  RTIOsapiSemaphore_take(self->mutex_self, RTI_NTP_TIME_INFINITE);

  if (DDS_RETCODE_OK != RTIROS2_Graph_convert_to_sample(self, self->pinfo))
  {
    /* TODO(asorbini) Log error */
    RTIOsapiSemaphore_give(self->mutex_self);
    goto done;
  }

  RTIOsapiSemaphore_give(self->mutex_self);

  if (DDS_RETCODE_OK != RTIROS2_Graph_publish_sample(self, self->pinfo))
  {
    /* TODO(asorbini) Log error */
    goto done;
  }
  retcode = DDS_RETCODE_OK;
  
done:
  return retcode;
}

DDS_ReturnCode_t
RTIROS2_CGraphSampleAdapter_publish_sample(
  RTIROS2_Graph * const self,
  void * const sample,
  void * const adapter_data)
{
  RTIROS2_ParticipantEntitiesInfoDataWriter * writer =
    RTIROS2_ParticipantEntitiesInfoDataWriter_narrow(self->graph_writer);
  RTIROS2_ParticipantEntitiesInfo * const pinfo =
    (RTIROS2_ParticipantEntitiesInfo*)sample;

  UNUSED_ARG(adapter_data);

  if (DDS_RETCODE_OK !=
    RTIROS2_ParticipantEntitiesInfoDataWriter_write(
      writer, pinfo, &DDS_HANDLE_NIL))
  {
    /* TODO(asorbini) Log error */
    return DDS_RETCODE_ERROR;
  }

  return DDS_RETCODE_OK;
}

void*
RTIROS2_CGraphSampleAdapter_alloc_sample(
  RTIROS2_Graph * const self,
  void * const adapter_data)
{
  UNUSED_ARG(self);
  UNUSED_ARG(adapter_data);
  return RTIROS2_ParticipantEntitiesInfoTypeSupport_create_data();
}

void
RTIROS2_CGraphSampleAdapter_free_sample(
  RTIROS2_Graph * const self,
  void * const sample,
  void * const adapter_data)
{
  UNUSED_ARG(self);
  UNUSED_ARG(adapter_data);
  RTIROS2_ParticipantEntitiesInfoTypeSupport_delete_data(sample);
}

void*
RTIROS2_Graph_update_thread(void * param)
{
  RTIROS2_Graph * const self = (RTIROS2_Graph *)param;
  RTIROS2_GraphNode * node = NULL;
  struct RTINtpTime ts_zero = RTI_NTP_TIME_ZERO;
  struct RTINtpTime ts_wait = RTI_NTP_TIME_ZERO;
  RTIOsapiSemaphoreStatus take_rc = RTI_OSAPI_SEMAPHORE_STATUS_ERROR;
  DDS_Boolean enable_poll = DDS_BOOLEAN_FALSE;

  RTINtpTime_packFromNanosec(ts_wait,
    self->poll_period.sec, self->poll_period.nanosec);

  enable_poll = RTIROS2_Graph_is_polling_enabled(self);

  while (self->thread_pinfo_active)
  {
    take_rc = RTIOsapiSemaphore_take(self->sem_pinfo,
      (enable_poll)?&ts_wait:RTI_NTP_TIME_INFINITE);

    if (!self->thread_pinfo_active)
    {
      continue;
    }

    /* TODO keep track of time of last poll, and poll if it's been more than
     "poll_period" since then. */
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

      take_rc = RTIOsapiSemaphore_take(self->sem_pinfo, &ts_zero);
    }

    if (RTI_OSAPI_SEMAPHORE_STATUS_OK == take_rc)
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

  return NULL;
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