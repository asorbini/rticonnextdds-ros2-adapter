// Copyright 2022 Real-Time Innovations, Inc. (RTI)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "rticonnextdds_ros2_adapter/rticonnextdds_ros2_adapter_cpp.hpp"

#ifndef UNUSED_ARG
#define UNUSED_ARG(arg_)        (void)(arg_)
#endif /* UNUSED_ARG */

namespace rti {

namespace ros2 {

const char * const GRAPH_TOPIC_NAME = RTIROS2_GRAPH_TOPIC_NAME;

const char * const GRAPH_TYPE_NAME = RTIROS2_GRAPH_TYPE_NAME;

static
GraphEndpointType
GraphEndpointType_from_c(const RTIROS2_GraphEndpointType_t c_type)
{
  switch (c_type)
  {
  case RTIROS2_GRAPH_ENDPOINT_SUBSCRIPTION:
  {
    return GraphEndpointType::Subscription;
  }
  case RTIROS2_GRAPH_ENDPOINT_PUBLISHER:
  {
    return GraphEndpointType::Publisher;
  }
  case RTIROS2_GRAPH_ENDPOINT_CLIENT:
  {
    return GraphEndpointType::Client;
  }
  case RTIROS2_GRAPH_ENDPOINT_SERVICE:
  {
    return GraphEndpointType::Service;
  }
  default:
  {
    return GraphEndpointType::Unknown;
  }
  }
}

#if 0
// Not used for anything at the moment
static
RTIROS2_GraphEndpointType_t
GraphEndpointType_to_c(const GraphEndpointType cpp_type)
{
  switch (cpp_type)
  {
  case GraphEndpointType::Subscription:
  {
    return RTIROS2_GRAPH_ENDPOINT_SUBSCRIPTION;
  }
  case GraphEndpointType::Publisher:
  {
    return RTIROS2_GRAPH_ENDPOINT_PUBLISHER;
  }
  case GraphEndpointType::Client:
  {
    return RTIROS2_GRAPH_ENDPOINT_CLIENT;
  }
  case GraphEndpointType::Service:
  {
    return RTIROS2_GRAPH_ENDPOINT_SERVICE;
  }
  default:
  {
    return RTIROS2_GRAPH_ENDPOINT_UNKNOWN;
  }
  }
}
#endif

static
DDS_ReturnCode_t
RTIROS2_CppGraphSampleAdapter_convert_to_sample(
  RTIROS2_Graph * const self,
  DDS_DomainParticipant * const dds_participant,
  void * const vsample,
  void * const adapter_data)
{
  UNUSED_ARG(self);
  Graph * cpp_graph = static_cast<Graph*>(adapter_data);
  RTIROS2::ParticipantEntitiesInfo * const sample =
    static_cast<RTIROS2::ParticipantEntitiesInfo*>(vsample);

  RTIROS2_ParticipantEntitiesInfo * const c_sample = cpp_graph->c_sample();
  if (nullptr == c_sample)
  {
    return DDS_RETCODE_ERROR;
  }

  if (DDS_RETCODE_OK !=
    RTIROS2_CGraphSampleAdapter_convert_to_sample(
      self, dds_participant, c_sample, nullptr))
  {
    return DDS_RETCODE_ERROR;
  }

  try {
    memcpy(&(sample->gid().data()[0]), c_sample->gid.data, RTIROS2::GID_LENGTH);
    const DDS_Long nodes_len = RTIROS2_NodeEntitiesInfoSeq_get_length(
      &c_sample->node_entities_info_seq);
    sample->node_entities_info_seq().resize(nodes_len);
    for (DDS_Long i = 0; i  < nodes_len; i++)
    {
      RTIROS2_NodeEntitiesInfo * const c_node =
        RTIROS2_NodeEntitiesInfoSeq_get_reference(
          &c_sample->node_entities_info_seq, i);
      RTIROS2::NodeEntitiesInfo * const cpp_node = 
        &(sample->node_entities_info_seq()[i]);
      cpp_node->node_name(c_node->node_name);
      cpp_node->node_namespace(c_node->node_namespace);

      const DDS_Long readers_len =
        RTIROS2_GidSeq_get_length(&c_node->reader_gid_seq);
      const DDS_Long writers_len =
        RTIROS2_GidSeq_get_length(&c_node->writer_gid_seq);

      cpp_node->reader_gid_seq().resize(readers_len);
      cpp_node->writer_gid_seq().resize(writers_len);

      for (DDS_Long j = 0; j < readers_len; j++)
      {
        RTIROS2_Gid * const c_gid = RTIROS2_GidSeq_get_reference(
          &c_node->reader_gid_seq, j);
        RTIROS2::Gid * const cpp_gid = &(cpp_node->reader_gid_seq()[j]);
        memcpy(&(cpp_gid->data()[0]), c_gid->data, RTIROS2::GID_LENGTH);
      }

      for (DDS_Long j = 0; j < writers_len; j++)
      {
        RTIROS2_Gid * const c_gid = RTIROS2_GidSeq_get_reference(
          &c_node->writer_gid_seq, j);
        RTIROS2::Gid * const cpp_gid = &cpp_node->writer_gid_seq()[j];
        memcpy(&(cpp_gid->data()[0]), c_gid->data, RTIROS2::GID_LENGTH);
      }
    }
  }
  catch (std::exception & e)
  {
    return DDS_RETCODE_ERROR;
  }

  return DDS_RETCODE_OK;
}

static
DDS_ReturnCode_t
RTIROS2_CppGraphSampleAdapter_publish_sample(
  RTIROS2_Graph * const self,
  void * const vsample,
  void * const adapter_data)
{
  Graph * cpp_graph = static_cast<Graph*>(adapter_data);
  RTIROS2::ParticipantEntitiesInfo * const sample =
    static_cast<RTIROS2::ParticipantEntitiesInfo*>(vsample);
  dds::pub::DataWriter<RTIROS2::ParticipantEntitiesInfo> writer =
    cpp_graph->graph_writer();

  UNUSED_ARG(self);

  try {
    writer->write(*sample);
  } catch (std::exception & e)
  {
    return DDS_RETCODE_ERROR;
  }

  return DDS_RETCODE_OK;
}

static
void*
RTIROS2_CppGraphSampleAdapter_alloc_sample(
  RTIROS2_Graph * const self,
  void * const adapter_data)
{
  UNUSED_ARG(self);
  UNUSED_ARG(adapter_data);
  return new (std::nothrow) RTIROS2::ParticipantEntitiesInfo();
}

static
void
RTIROS2_CppGraphSampleAdapter_free_sample(
  RTIROS2_Graph * const self,
  void * const vsample,
  void * const adapter_data)
{
  UNUSED_ARG(self);
  UNUSED_ARG(adapter_data);
  RTIROS2::ParticipantEntitiesInfo * const sample =
    static_cast<RTIROS2::ParticipantEntitiesInfo*>(vsample);
  delete sample;
}

namespace graph {
void
customize_datawriter_qos(
  dds::pub::qos::DataWriterQos & writer_qos)
{
  if (DDS_RETCODE_OK !=
    RTIROS2_Graph_customize_datawriter_qos(writer_qos->native()))
  {
    throw dds::core::Error("failed to customize writer qos");
  }
}

void
customize_datareader_qos(
  dds::sub::qos::DataReaderQos & reader_qos)
{
  if (DDS_RETCODE_OK !=
    RTIROS2_Graph_customize_datareader_qos(reader_qos->native()))
  {
    throw dds::core::Error("failed to customize reader qos");
  }
}

void
compute_participant_gid(
  dds::domain::DomainParticipant & dds_participant,
  RTIROS2::Gid & gid)
{
  RTIROS2_Gid c_gid;
  if (DDS_RETCODE_OK !=
    RTIROS2_Graph_compute_participant_gid(
      dds_participant->native_participant(), &c_gid))
  {
    throw dds::core::Error("failed to convert c gid");
  }
  memcpy(&(gid.data()[0]), c_gid.data, RTIROS2::GID_LENGTH);
}

void
compute_reader_gid(
  dds::sub::AnyDataReader & dds_reader,
  RTIROS2::Gid & gid)
{
  RTIROS2_Gid c_gid;
  if (DDS_RETCODE_OK !=
    RTIROS2_Graph_compute_reader_gid(dds_reader->native_reader(), &c_gid))
  {
    throw dds::core::Error("failed to convert c gid");
  }
  memcpy(&(gid.data()[0]), c_gid.data, RTIROS2::GID_LENGTH);
}

void
compute_writer_gid(
  dds::pub::AnyDataWriter & dds_writer,
  RTIROS2::Gid & gid)
{
  RTIROS2_Gid c_gid;
  if (DDS_RETCODE_OK !=
    RTIROS2_Graph_compute_writer_gid(dds_writer->native_writer(), &c_gid))
  {
    throw dds::core::Error("failed to convert c gid");
  }
  memcpy(&(gid.data()[0]), c_gid.data, RTIROS2::GID_LENGTH);
}

void
compute_publisher_topic_names(
  const std::string & ros2_topic_name,
  const std::string & ros2_type_name,
  std::string & dds_topic_name,
  std::string & dds_type_name)
{
  size_t dds_topic_name_len = 0;
  size_t dds_type_name_len = 0;

  if (DDS_RETCODE_OK !=
    RTIROS2_Graph_compute_publisher_topic_names(
      ros2_topic_name.c_str(),
      ros2_type_name.c_str(),
      nullptr,
      &dds_topic_name_len,
      nullptr,
      &dds_type_name_len))
  {
    throw dds::core::Error("failed to query size of writer topic names");
  }

  char * topic_name_buf = DDS_String_alloc(dds_topic_name_len - 1);
  if (nullptr == topic_name_buf)
  {
    throw dds::core::Error("failed to allocate topic name string");
  }

  char * type_name_buf = DDS_String_alloc(dds_type_name_len - 1);
  if (nullptr == topic_name_buf)
  {
    DDS_String_free(topic_name_buf);
    throw dds::core::Error("failed to allocate type name string");
  }

  if (DDS_RETCODE_OK !=
    RTIROS2_Graph_compute_publisher_topic_names(
      ros2_topic_name.c_str(),
      ros2_type_name.c_str(),
      topic_name_buf,
      &dds_topic_name_len,
      type_name_buf,
      &dds_type_name_len))
  {
    DDS_String_free(topic_name_buf);
    DDS_String_free(type_name_buf);
    throw dds::core::Error("failed to generate writer topic names");
  }

  dds_topic_name = topic_name_buf;
  dds_type_name = type_name_buf;

  DDS_String_free(topic_name_buf);
  DDS_String_free(type_name_buf);
}

void
compute_subscription_topic_names(
  const std::string & ros2_topic_name,
  const std::string & ros2_type_name,
  std::string & dds_topic_name,
  std::string & dds_type_name)
{
  size_t dds_topic_name_len = 0;
  size_t dds_type_name_len = 0;

  if (DDS_RETCODE_OK !=
    RTIROS2_Graph_compute_subscription_topic_names(
      ros2_topic_name.c_str(),
      ros2_type_name.c_str(),
      nullptr,
      &dds_topic_name_len,
      nullptr,
      &dds_type_name_len))
  {
    throw dds::core::Error("failed to query size of reader topic names");
  }

  char * topic_name_buf = DDS_String_alloc(dds_topic_name_len - 1);
  if (nullptr == topic_name_buf)
  {
    throw dds::core::Error("failed to allocate topic name string");
  }

  char * type_name_buf = DDS_String_alloc(dds_type_name_len - 1);
  if (nullptr == topic_name_buf)
  {
    DDS_String_free(topic_name_buf);
    throw dds::core::Error("failed to allocate type name string");
  }

  if (DDS_RETCODE_OK !=
    RTIROS2_Graph_compute_subscription_topic_names(
      ros2_topic_name.c_str(),
      ros2_type_name.c_str(),
      topic_name_buf,
      &dds_topic_name_len,
      type_name_buf,
      &dds_type_name_len))
  {
    DDS_String_free(topic_name_buf);
    DDS_String_free(type_name_buf);
    throw dds::core::Error("failed to generate reader topic names");
  }

  dds_topic_name = topic_name_buf;
  dds_type_name = type_name_buf;

  DDS_String_free(topic_name_buf);
  DDS_String_free(type_name_buf);
}

void
compute_service_topic_names(
  const std::string & ros2_node_name,
  const std::string & ros2_service_name,
  const std::string & ros2_type_name,
  std::string & dds_req_topic_name,
  std::string & dds_req_type_name,
  std::string & dds_rep_topic_name,
  std::string & dds_rep_type_name)
{
  size_t dds_req_topic_name_len = 0;
  size_t dds_req_type_name_len = 0;
  size_t dds_rep_topic_name_len = 0;
  size_t dds_rep_type_name_len = 0;

  if (DDS_RETCODE_OK !=
    RTIROS2_Graph_compute_service_topic_names(
      ros2_node_name.c_str(),
      ros2_service_name.c_str(),
      ros2_type_name.c_str(),
      nullptr,
      &dds_req_topic_name_len,
      nullptr,
      &dds_req_type_name_len,
      nullptr,
      &dds_rep_topic_name_len,
      nullptr,
      &dds_rep_type_name_len))
  {
    throw dds::core::Error("failed to query size of service topic names");
  }

  char * req_topic_name_buf = DDS_String_alloc(dds_req_topic_name_len - 1);
  if (nullptr == req_topic_name_buf)
  {
    throw dds::core::Error("failed to allocate request topic name string");
  }

  char * req_type_name_buf = DDS_String_alloc(dds_req_type_name_len - 1);
  if (nullptr == req_type_name_buf)
  {
    DDS_String_free(req_topic_name_buf);
    throw dds::core::Error("failed to allocate request type name string");
  }

  char * rep_topic_name_buf = DDS_String_alloc(dds_rep_topic_name_len - 1);
  if (nullptr == rep_topic_name_buf)
  {
    DDS_String_free(req_topic_name_buf);
    DDS_String_free(req_type_name_buf);
    throw dds::core::Error("failed to allocate reply topic name string");
  }

  char * rep_type_name_buf = DDS_String_alloc(dds_rep_type_name_len - 1);
  if (nullptr == rep_type_name_buf)
  {
    DDS_String_free(req_topic_name_buf);
    DDS_String_free(req_type_name_buf);
    DDS_String_free(rep_topic_name_buf);
    throw dds::core::Error("failed to allocate reply type name string");
  }

  if (DDS_RETCODE_OK !=
    RTIROS2_Graph_compute_service_topic_names(
      ros2_node_name.c_str(),
      ros2_service_name.c_str(),
      ros2_type_name.c_str(),
      req_topic_name_buf,
      &dds_req_topic_name_len,
      req_type_name_buf,
      &dds_req_type_name_len,
      rep_topic_name_buf,
      &dds_rep_topic_name_len,
      req_type_name_buf,
      &dds_rep_type_name_len))
  {
    DDS_String_free(req_topic_name_buf);
    DDS_String_free(req_type_name_buf);
    DDS_String_free(rep_topic_name_buf);
    DDS_String_free(rep_type_name_buf);
    throw dds::core::Error("failed to generate reader topic names");
  }

  dds_req_topic_name = req_topic_name_buf;
  dds_req_type_name = req_type_name_buf;
  dds_rep_topic_name = rep_topic_name_buf;
  dds_rep_type_name = rep_type_name_buf;

  DDS_String_free(req_topic_name_buf);
  DDS_String_free(req_type_name_buf);
  DDS_String_free(rep_topic_name_buf);
  DDS_String_free(rep_type_name_buf);
}

}  // namespace graph

const GraphNodeHandle GraphNodeHandle_INVALID = RTIROS2_GraphNodeHandle_INVALID;

const GraphEndpointHandle GraphEndpointHandle_INVALID =
  RTIROS2_GraphEndpointHandle_INVALID;

Graph::Graph(const GraphProperties & properties)
: graph_participant_(properties.graph_participant),
  graph_publisher_(properties.graph_publisher),
  graph_topic_(properties.graph_topic),
  graph_writer_(properties.graph_writer),
  poll_period_(properties.poll_period)
{
  if (nullptr == graph_participant_)
  {
    throw dds::core::InvalidArgumentError("no domain participant");
  }

  if (nullptr == graph_publisher_)
  {
    graph_publisher_ = dds::pub::Publisher(graph_participant_);
  }

  if (nullptr == graph_topic_)
  {
    // Check if topic already exist outherwise create it
    graph_topic_ =
      dds::topic::find<dds::topic::Topic<RTIROS2::ParticipantEntitiesInfo>>(
        graph_participant_, GRAPH_TOPIC_NAME);
    if (nullptr == graph_topic_)
    {
      graph_topic_ = dds::topic::Topic<RTIROS2::ParticipantEntitiesInfo>(
        graph_participant_, GRAPH_TOPIC_NAME, GRAPH_TYPE_NAME);
    }
  }

  if (nullptr == graph_writer_)
  {
    dds::pub::qos::DataWriterQos writer_qos;

    if (DDS_RETCODE_OK !=
      DDS_Publisher_get_default_datawriter_qos_w_topic_name(
        graph_publisher_->native_publisher(),
        writer_qos->native(),
        GRAPH_TOPIC_NAME))
    {
      throw std::runtime_error("failed to get C writer qos");
    }

    rti::ros2::graph::customize_datawriter_qos(writer_qos);

    graph_writer_ =
      dds::pub::DataWriter<RTIROS2::ParticipantEntitiesInfo>(
        graph_publisher_, graph_topic_, writer_qos);
  }

  c_pinfo_ = RTIROS2_ParticipantEntitiesInfoTypeSupport_create_data();
  if (nullptr == c_pinfo_)
  {
    throw dds::core::Error("failed to allocate C sample");
  }

  RTIROS2_GraphProperties c_props = RTIROS2_GraphProperties_INITIALIZER;
  c_props.graph_participant = graph_participant_->native_participant();
  c_props.graph_publisher = graph_publisher_->native_publisher();
  c_props.graph_topic = graph_topic_->native_topic();
  c_props.graph_writer = graph_writer_->native_writer();
  c_props.sample_adapter.convert_to_sample =
    RTIROS2_CppGraphSampleAdapter_convert_to_sample;
  c_props.sample_adapter.publish_sample =
    RTIROS2_CppGraphSampleAdapter_publish_sample;
  c_props.sample_adapter.alloc_sample =
    RTIROS2_CppGraphSampleAdapter_alloc_sample;
  c_props.sample_adapter.free_sample =
    RTIROS2_CppGraphSampleAdapter_free_sample;
  c_props.sample_adapter.adapter_data = this;

  c_graph_ = RTIROS2_Graph_new(&c_props);
  if (nullptr == c_graph_)
  {
    throw dds::core::Error("failed to initialize C graph");
  }
}


Graph::~Graph()
{
  RTIROS2_Graph_delete(c_graph_);
  RTIROS2_ParticipantEntitiesInfoTypeSupport_delete_data(c_pinfo_);
}


dds::domain::DomainParticipant
Graph::graph_participant() const
{
  return graph_participant_;
}


dds::pub::Publisher
Graph::graph_publisher() const
{
  return graph_publisher_;
}


dds::topic::Topic<RTIROS2::ParticipantEntitiesInfo>
Graph::graph_topic() const
{
  return graph_topic_;
}


dds::pub::DataWriter<RTIROS2::ParticipantEntitiesInfo>
Graph::graph_writer() const
{
  return graph_writer_;
}

GraphNodeHandle
Graph::register_local_node(
  const std::string & node_name,
  const std::string & node_namespace,
  dds::domain::DomainParticipant dds_participant)
{
  if (nullptr == dds_participant)
  {
    dds_participant = graph_participant_;
  }
  return RTIROS2_Graph_register_local_node(c_graph_,
    node_name.c_str(),
    (node_namespace.length() > 0)? node_namespace.c_str():nullptr,
    dds_participant->native_participant());
}

void
Graph::inspect_local_node(
  const GraphNodeHandle node_handle)
{
  if (DDS_RETCODE_OK != RTIROS2_Graph_inspect_local_node(c_graph_, node_handle))
  {
    throw dds::core::Error("failed to inspect node");
  }
}

GraphEndpointHandle
Graph::register_local_subscription(
  const GraphNodeHandle node_handle,
  dds::sub::AnyDataReader sub_reader)
{
  return RTIROS2_Graph_register_local_subscription(
    c_graph_, node_handle, sub_reader->native_reader());
}

GraphEndpointHandle
Graph::register_local_publisher(
  const GraphNodeHandle node_handle,
  dds::pub::AnyDataWriter pub_writer)
{
  return RTIROS2_Graph_register_local_publisher(
    c_graph_, node_handle, pub_writer->native_writer());
}

GraphEndpointHandle
Graph::register_local_client(
  const GraphNodeHandle node_handle,
  dds::sub::AnyDataReader client_reader,
  dds::pub::AnyDataWriter client_writer)
{
  return RTIROS2_Graph_register_local_client(
    c_graph_,
    node_handle,
    client_reader->native_reader(),
    client_writer->native_writer());
}

GraphEndpointHandle
Graph::register_local_service(
  const GraphNodeHandle node_handle,
  dds::sub::AnyDataReader service_reader,
  dds::pub::AnyDataWriter service_writer)
{
  return RTIROS2_Graph_register_local_service(
    c_graph_,
    node_handle,
    service_reader->native_reader(),
    service_writer->native_writer());
}

void
Graph::unregister_local_node(const GraphNodeHandle node_handle)
{
  if (DDS_RETCODE_OK !=
    RTIROS2_Graph_unregister_local_node(c_graph_, node_handle))
  {
    throw dds::core::Error("failed to unregiter local node");
  }
}

void
Graph::unregister_local_subscription(
  const RTIROS2_GraphNodeHandle node_handle,
  dds::sub::AnyDataReader sub_reader)
{
  if (DDS_RETCODE_OK !=
    RTIROS2_Graph_unregister_local_subscription(
      c_graph_, node_handle, sub_reader->native_reader()))
  {
    throw dds::core::Error("failed to unregiter local subscription");
  }
}

void
Graph::unregister_local_publisher(
  const GraphNodeHandle node_handle,
  dds::pub::AnyDataWriter pub_writer)
{
  if (DDS_RETCODE_OK !=
    RTIROS2_Graph_unregister_local_publisher(
      c_graph_, node_handle, pub_writer->native_writer()))
  {
    throw dds::core::Error("failed to unregiter local publisher");
  }
}

void
Graph::unregister_local_client(
  const GraphNodeHandle node_handle,
  dds::sub::AnyDataReader client_reader,
  dds::pub::AnyDataWriter client_writer)
{
  if (DDS_RETCODE_OK !=
    RTIROS2_Graph_unregister_local_client(
      c_graph_,
      node_handle,
      client_reader->native_reader(),
      client_writer->native_writer()))
  {
    throw dds::core::Error("failed to unregiter local client");
  }
}

void
Graph::unregister_local_service(
  const GraphNodeHandle node_handle,
  dds::sub::AnyDataReader service_reader,
  dds::pub::AnyDataWriter service_writer)
{
  if (DDS_RETCODE_OK !=
    RTIROS2_Graph_unregister_local_service(
      c_graph_,
      node_handle,
      service_reader->native_reader(),
      service_writer->native_writer()))
  {
    throw dds::core::Error("failed to unregiter local service");
  }
}

void
Graph::unregister_local_subscription(
  const GraphNodeHandle node_handle,
  const GraphEndpointHandle sub_handle)
{
  if (DDS_RETCODE_OK !=
    RTIROS2_Graph_unregister_local_subscription_by_handle(
      c_graph_, node_handle, sub_handle))
  {
    throw dds::core::Error("failed to unregiter local subscription");
  }
}

void
Graph::unregister_local_publisher(
  const GraphNodeHandle node_handle,
  const GraphEndpointHandle pub_handle)
{
  if (DDS_RETCODE_OK !=
    RTIROS2_Graph_unregister_local_publisher_by_handle(
      c_graph_, node_handle, pub_handle))
  {
    throw dds::core::Error("failed to unregiter local publisher");
  }
}

void
Graph::unregister_local_client(
  const GraphNodeHandle node_handle,
  const GraphEndpointHandle client_handle)
{
  if (DDS_RETCODE_OK !=
    RTIROS2_Graph_unregister_local_client_by_handle(
      c_graph_, node_handle, client_handle))
  {
    throw dds::core::Error("failed to unregiter local client");
  }
}

void
Graph::unregister_local_service_by_handle(
  const GraphNodeHandle node_handle,
  const GraphEndpointHandle service_handle)
{
  if (DDS_RETCODE_OK !=
    RTIROS2_Graph_unregister_local_service_by_handle(
      c_graph_, node_handle, service_handle))
  {
    throw dds::core::Error("failed to unregiter local service");
  }
}

dds::domain::DomainParticipant
Graph::get_node_participant(const GraphNodeHandle node_handle)
{
  DDS_DomainParticipant * c_participant =
    RTIROS2_Graph_get_node_participant(c_graph_, node_handle);
  if (nullptr == c_participant)
  {
    throw dds::core::Error("failed to get node participant");
  }
  return rti::core::detail::create_from_native_entity<
    dds::domain::DomainParticipant, DDS_DomainParticipant>(c_participant);
}

void
Graph::get_node_name(
  const GraphNodeHandle node_handle,
  const char ** const name_out,
  const char ** const namespace_out)
{
  if (DDS_RETCODE_OK !=
    RTIROS2_Graph_get_node_name(
      c_graph_, node_handle, name_out, namespace_out))
  {
    throw dds::core::Error("failed to get node name");
  }
}

dds::core::optional<dds::sub::AnyDataReader>
Graph::get_endpoint_reader(
  const GraphNodeHandle node_handle,
  const GraphEndpointHandle endp_handle)
{
  DDS_DataReader * c_reader =
    RTIROS2_Graph_get_endpoint_reader(c_graph_, node_handle, endp_handle);
  if (nullptr == c_reader)
  {
    return dds::core::optional<dds::sub::AnyDataReader>();
  }
  return dds::core::optional<dds::sub::AnyDataReader>(
    rti::core::detail::create_from_native_entity<
      dds::sub::AnyDataReader, DDS_DataReader>(c_reader));
}

dds::core::optional<dds::pub::AnyDataWriter>
Graph::get_endpoint_writer(
  const GraphNodeHandle node_handle,
  const GraphEndpointHandle endp_handle)
{
  DDS_DataWriter * c_writer =
    RTIROS2_Graph_get_endpoint_writer(c_graph_, node_handle, endp_handle);
  if (nullptr == c_writer)
  {
    return dds::core::optional<dds::pub::AnyDataWriter>();
  }
  return dds::core::optional<dds::pub::AnyDataWriter>(
    rti::core::detail::create_from_native_entity<
      dds::pub::AnyDataWriter, DDS_DataWriter>(c_writer));
}

GraphEndpointType
Graph::get_endpoint_type(
  const GraphNodeHandle node_handle,
  const GraphEndpointHandle endp_handle)
{
  return GraphEndpointType_from_c(
    RTIROS2_Graph_get_endpoint_type(c_graph_, node_handle, endp_handle));
}

}  // namespace ros2
}  // namespace rti