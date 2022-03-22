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

const char * const RTIROS2_GRAPH_TOPIC_NAME = "ros_discovery_info";

const char * const RTIROS2_GRAPH_TYPE_NAME =
  "rmw_dds_common::msg::dds::ParticipantEntitiesInfo_";

RTIROS2_Graph *
RTIROS2_Graph_new(
  const struct RTIROS2_GraphProperties * const properties)
{
  DDS_ReturnCode_t retcode = DDS_RETCODE_ERROR;
  RTIROS2_Graph * result = NULL;
  
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
      RTIOsapiHeap_free(result);
      result = NULL;
    }
  }
  return result;
}

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
      rmw_dds_common_msg_ParticipantEntitiesInfoTypeSupport_register_type(
        self->graph_participant, RTIROS2_GRAPH_TYPE_NAME))
    {
      /* TODO(asorbini) Log error */
      goto done;
    }

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
    self->graph_topic_own = DDS_BOOLEAN_TRUE;
  }

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
      RTIROS2_Graph_delete(self);
    }
  }
  DDS_DataWriterQos_finalize(&graph_writer_qos);
  return retcode;
}

void
RTIROS2_Graph_delete(RTIROS2_Graph * const self)
{
  RTIROS2_Graph_finalize(self);
  RTIOsapiHeap_free(self);
}

DDS_ReturnCode_t
RTIROS2_Graph_finalize(RTIROS2_Graph * const self)
{
  DDS_ReturnCode_t retcode = DDS_RETCODE_ERROR;
  RTIROS2_GraphNode *node =
    (RTIROS2_GraphNode*) REDAInlineList_getFirst(&self->nodes);

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
  DDS_DomainParticipant * node_participant = NULL;
  const DDS_Boolean enable_poll = RTIROS2_Graph_is_polling_enabled(self);

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

  if (NULL != node_namespace)
  {
    node->node_namespace = DDS_String_dup(node_namespace);
    if (NULL == node->node_namespace)
    {
      /* TODO(asorbini) Log error */
      goto done;
    }
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
  
  RTIOsapiSemaphore_take(self->mutex_self, RTI_NTP_TIME_INFINITE);

  node = RTIROS2_Graph_lookup_local_node_by_handle(self, node_handle);
  if (NULL == node)
  {
    /* TODO(asorbini) Log error */
    goto done;
  }

  retcode = RTIROS2_Graph_inspect_local_nodeEA(self, node);
  
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
  RTIOsapiSemaphore_take(self->mutex_self, RTI_NTP_TIME_INFINITE);
  result = RTIROS2_Graph_register_local_subscriptionEA(
    self, node_handle, sub_reader);
  RTIOsapiSemaphore_give(self->mutex_self);
  return result;
}

RTIROS2_GraphEndpointHandle
RTIROS2_Graph_register_local_publication(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle,
  DDS_DataWriter * const pub_writer)
{
  RTIROS2_GraphEndpointHandle result = RTIROS2_GraphEndpointHandle_INVALID;
  RTIOsapiSemaphore_take(self->mutex_self, RTI_NTP_TIME_INFINITE);
  result = RTIROS2_Graph_register_local_publisherEA(
    self, node_handle, pub_writer);
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
  RTIOsapiSemaphore_take(self->mutex_self, RTI_NTP_TIME_INFINITE);
  result = RTIROS2_Graph_register_local_clientEA(
    self, node_handle, client_reader, client_writer);
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
  RTIOsapiSemaphore_take(self->mutex_self, RTI_NTP_TIME_INFINITE);
  result = RTIROS2_Graph_register_local_serviceEA(
    self, node_handle, service_reader, service_writer);
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
  RTIROS2_GraphEndpoint * endp = NULL;

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
  return RTIROS2_Graph_unregister_local_endpoint(self, node_handle, sub_reader, NULL);
}

DDS_ReturnCode_t
RTIROS2_Graph_unregister_local_publisher(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle,
  DDS_DataWriter * const pub_writer)
{
  return RTIROS2_Graph_unregister_local_endpoint(self, node_handle, NULL, pub_writer);
}

DDS_ReturnCode_t
RTIROS2_Graph_unregister_local_client(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle,
  DDS_DataReader * const client_reader,
  DDS_DataWriter * const client_writer)
{
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
  return RTIROS2_Graph_unregister_local_endpoint(
    self, node_handle, service_reader, service_writer);
}


DDS_ReturnCode_t
RTIROS2_Graph_unregister_local_subscription_by_handle(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle,
  const RTIROS2_GraphEndpointHandle sub_handle)
{
  return RTIROS2_Graph_unregister_local_endpoint_by_handle(
    self, node_handle, sub_handle);
}

DDS_ReturnCode_t
RTIROS2_Graph_unregister_local_publisher_by_handle(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle,
  const RTIROS2_GraphEndpointHandle pub_handle)
{
  return RTIROS2_Graph_unregister_local_endpoint_by_handle(
    self, node_handle, pub_handle);
}

DDS_ReturnCode_t
RTIROS2_Graph_unregister_local_client_by_handle(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle,
  const RTIROS2_GraphEndpointHandle client_handle)
{
  return RTIROS2_Graph_unregister_local_endpoint_by_handle(
    self, node_handle, client_handle);
}

DDS_ReturnCode_t
RTIROS2_Graph_unregister_local_service_by_handle(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle,
  const RTIROS2_GraphEndpointHandle service_handle)
{
  return RTIROS2_Graph_unregister_local_endpoint_by_handle(
    self, node_handle, service_handle);
}

DDS_ReturnCode_t
RTIROS2_Graph_inspect_local_participant(
  RTIROS2_Graph * const self,
  const RTIROS2_GraphNodeHandle node_handle,
  DDS_DomainParticipant * const dds_participant)
{
  DDS_ReturnCode_t retcode = DDS_RETCODE_ERROR;



  retcode = DDS_RETCODE_OK;
  
done:
  return retcode;
}