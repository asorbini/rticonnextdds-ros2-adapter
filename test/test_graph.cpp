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
#include <thread>
#include <chrono>

#include <gtest/gtest.h>

#include "rticonnextdds_ros2_adapter/rticonnextdds_ros2_adapter_graph.h"

#include "test_model.h"

using namespace std::chrono_literals;

class TestGraph : public ::testing::Test {
 protected:
  void SetUp() override {
    dds_factory = DDS_DomainParticipantFactory_get_instance();
    ASSERT_NE(dds_factory, nullptr);

    DDS_ReturnCode_t rc =
      DDS_DomainParticipantFactory_get_default_participant_qos(
        dds_factory, &dp_qos);
    ASSERT_EQ(rc, DDS_RETCODE_OK);
    dp_qos.database.shutdown_cleanup_period.sec = 0;
    dp_qos.database.shutdown_cleanup_period.nanosec = 10000000;

    graph_participant = DDS_DomainParticipantFactory_create_participant(
      dds_factory, 0, &dp_qos, NULL, DDS_STATUS_MASK_NONE);
    ASSERT_NE(graph_participant, nullptr);

    another_participant = DDS_DomainParticipantFactory_create_participant(
      dds_factory, 1, &dp_qos, NULL, DDS_STATUS_MASK_NONE);
    ASSERT_NE(another_participant, nullptr);
  }

  void TearDown() override {
    DDS_ReturnCode_t rc =
      DDS_DomainParticipant_delete_contained_entities(graph_participant);
    ASSERT_EQ(rc, DDS_RETCODE_OK);

    rc = DDS_DomainParticipantFactory_delete_participant(
      dds_factory, graph_participant);
    ASSERT_EQ(rc, DDS_RETCODE_OK);

    rc =
      DDS_DomainParticipant_delete_contained_entities(another_participant);
    ASSERT_EQ(rc, DDS_RETCODE_OK);

    rc = DDS_DomainParticipantFactory_delete_participant(
      dds_factory, another_participant);
    ASSERT_EQ(rc, DDS_RETCODE_OK);

    DDS_DomainParticipantFactory_finalize_instance();
    DDS_DomainParticipantQos_finalize(&dp_qos);
  }

  DDS_DomainParticipantFactory * dds_factory{nullptr};
  DDS_DomainParticipant * graph_participant{nullptr};
  DDS_DomainParticipant * another_participant{nullptr};
  DDS_DomainParticipantQos dp_qos = DDS_DomainParticipantQos_INITIALIZER;
};

TEST_F(TestGraph, new_graph) {
  RTIROS2_GraphProperties props = RTIROS2_GraphProperties_INITIALIZER;
  props.graph_participant = graph_participant;

  RTIROS2_Graph * graph = RTIROS2_Graph_new(&props);
  ASSERT_NE(graph, nullptr);

  DDS_DomainParticipant * g_participant =
    RTIROS2_Graph_get_graph_participant(graph);
  ASSERT_EQ(g_participant, graph_participant);

  DDS_Publisher * g_publisher = RTIROS2_Graph_get_graph_publisher(graph);
  ASSERT_NE(g_publisher, nullptr);

  DDS_Topic * g_topic = RTIROS2_Graph_get_graph_topic(graph);
  ASSERT_NE(g_topic, nullptr);

  DDS_DataWriter * g_writer = RTIROS2_Graph_get_graph_writer(graph);
  ASSERT_NE(g_writer, nullptr);

  // We can create more graphs if needed.
  RTIROS2_Graph * another_graph = RTIROS2_Graph_new(&props);
  ASSERT_NE(another_graph, nullptr);

  DDS_DomainParticipant * ag_participant =
    RTIROS2_Graph_get_graph_participant(another_graph);
  ASSERT_EQ(ag_participant, g_participant);

  DDS_Publisher * ag_publisher =
    RTIROS2_Graph_get_graph_publisher(another_graph);
  ASSERT_NE(ag_publisher, nullptr);
  ASSERT_NE(ag_publisher, g_publisher);

  DDS_Topic * ag_topic = RTIROS2_Graph_get_graph_topic(another_graph);
  ASSERT_EQ(ag_topic, g_topic);

  DDS_DataWriter * ag_writer = RTIROS2_Graph_get_graph_writer(another_graph);
  ASSERT_NE(ag_writer, nullptr);
  ASSERT_NE(ag_writer, g_writer);

  RTIROS2_Graph_delete(graph);
  RTIROS2_Graph_delete(another_graph);
}

TEST_F(TestGraph, new_graph_bad_arguments) {
  RTIROS2_GraphProperties props = RTIROS2_GraphProperties_INITIALIZER;
  
  // properties must be non-NULL
  RTIROS2_Graph * graph = RTIROS2_Graph_new(nullptr);
  ASSERT_EQ(graph, nullptr);

  // properties must include at least a DomainParticipant.
  graph = RTIROS2_Graph_new(&props);
  ASSERT_EQ(graph, nullptr);

  // delete() also handles NULL arguments (albeit with no return value)
  RTIROS2_Graph_delete(nullptr);
}

TEST_F(TestGraph, register_local_node) {
  RTIROS2_GraphProperties props = RTIROS2_GraphProperties_INITIALIZER;
  props.graph_participant = graph_participant;
  RTIROS2_Graph * graph = RTIROS2_Graph_new(&props);
  ASSERT_NE(graph, nullptr);

  // Namespace might be NULL
  RTIROS2_GraphNodeHandle node_handle_1 = RTIROS2_Graph_register_local_node(
    graph, "foo", nullptr, another_participant);
  ASSERT_NE(node_handle_1, RTIROS2_GraphNodeHandle_INVALID);

  DDS_DomainParticipant * node_participant =
    RTIROS2_Graph_get_node_participant(graph, node_handle_1);
  ASSERT_EQ(node_participant, another_participant);

  // or a non-empty namespace might be specified
  RTIROS2_GraphNodeHandle node_handle_2 =
    RTIROS2_Graph_register_local_node(graph, "foo", "bar", graph_participant);
  ASSERT_NE(node_handle_2, RTIROS2_GraphNodeHandle_INVALID);
  node_participant = RTIROS2_Graph_get_node_participant(graph, node_handle_2);
  ASSERT_EQ(node_participant, graph_participant);

  // The participant is optional, in which case the graph's own participant is
  // used.
  RTIROS2_GraphNodeHandle node_handle_3 =
    RTIROS2_Graph_register_local_node(graph, "foo1", nullptr, nullptr);
  ASSERT_NE(node_handle_3, RTIROS2_GraphNodeHandle_INVALID);
  node_participant = RTIROS2_Graph_get_node_participant(graph, node_handle_3);
  ASSERT_EQ(node_participant, graph_participant);

  // We cannot register a node that has already been registered
  RTIROS2_GraphNodeHandle node_handle_4 =
    RTIROS2_Graph_register_local_node(graph, "foo", nullptr, another_participant);
  ASSERT_EQ(node_handle_4, RTIROS2_GraphNodeHandle_INVALID);

  node_handle_4 = RTIROS2_Graph_register_local_node(
    graph, "foo", "bar", graph_participant);
  ASSERT_EQ(node_handle_4, RTIROS2_GraphNodeHandle_INVALID);

  node_handle_4 = RTIROS2_Graph_register_local_node(
    graph, "foo1", nullptr, nullptr);
  ASSERT_EQ(node_handle_4, RTIROS2_GraphNodeHandle_INVALID);

  RTIROS2_Graph_delete(graph);
}

TEST_F(TestGraph, register_local_node_bad_args) {
  RTIROS2_GraphProperties props = RTIROS2_GraphProperties_INITIALIZER;
  props.graph_participant = graph_participant;
  RTIROS2_Graph * graph = RTIROS2_Graph_new(&props);
  ASSERT_NE(graph, nullptr);

  // Graph must be non-NULL
  RTIROS2_GraphNodeHandle node_handle =
    RTIROS2_Graph_register_local_node(NULL, "foo", NULL, graph_participant);
  ASSERT_EQ(node_handle, RTIROS2_GraphNodeHandle_INVALID);

  // Same for name, which also must be non-empty
  node_handle =
    RTIROS2_Graph_register_local_node(graph, nullptr, nullptr, graph_participant);
  ASSERT_EQ(node_handle, RTIROS2_GraphNodeHandle_INVALID);

  node_handle =
    RTIROS2_Graph_register_local_node(graph, "", nullptr, graph_participant);
  ASSERT_EQ(node_handle, RTIROS2_GraphNodeHandle_INVALID);

  // namespace must be non-empty if specified
  node_handle =
    RTIROS2_Graph_register_local_node(graph, "foo", "", graph_participant);
  ASSERT_EQ(node_handle, RTIROS2_GraphNodeHandle_INVALID);

  RTIROS2_Graph_delete(graph);
}

class TestGraphNode : public TestGraph {
 protected:
  void SetUp() override {
    TestGraph::SetUp();
    RTIROS2_GraphProperties props = RTIROS2_GraphProperties_INITIALIZER;
    props.graph_participant = graph_participant;

    graph = RTIROS2_Graph_new(&props);
    ASSERT_NE(graph, nullptr);

    node_handle =
      RTIROS2_Graph_register_local_node(graph, "foo", nullptr, nullptr);
    ASSERT_NE(node_handle, RTIROS2_GraphNodeHandle_INVALID);
  }

  void TearDown() override {
    RTIROS2_Graph_delete(graph);
    TestGraph::TearDown();
  }

  RTIROS2_Graph * graph;
  RTIROS2_GraphNodeHandle node_handle;
};

class TestGraphEndpoints : public TestGraphNode {
 protected:
  void SetUp() override {
    TestGraphNode::SetUp();
    DDS_ReturnCode_t rc =
      RTIROS2_StringTypeSupport_register_type(graph_participant, "String");
    ASSERT_EQ(rc, DDS_RETCODE_OK);

    topic = DDS_DomainParticipant_create_topic(graph_participant,
      "foo", "String", &DDS_TOPIC_QOS_DEFAULT, NULL, DDS_STATUS_MASK_NONE);
    ASSERT_NE(topic, nullptr);

    sub_reader = create_reader();
    ASSERT_NE(sub_reader, nullptr);
    pub_writer = create_writer();
    ASSERT_NE(pub_writer, nullptr);

    client_reader = create_reader();
    ASSERT_NE(client_reader, nullptr);
    client_writer = create_writer();
    ASSERT_NE(client_writer, nullptr);

    service_reader = create_reader();
    ASSERT_NE(service_reader, nullptr);
    service_writer = create_writer();
    ASSERT_NE(service_writer, nullptr);
  }

  void TearDown() override {
    TestGraphNode::TearDown();
  }

  DDS_DataReader*
  create_reader()
  {
    return DDS_DomainParticipant_create_datareader(
      graph_participant, DDS_Topic_as_topicdescription(topic),
        &DDS_DATAREADER_QOS_DEFAULT, NULL, DDS_STATUS_MASK_NONE);
  }

  DDS_DataWriter*
  create_writer()
  {
    return DDS_DomainParticipant_create_datawriter(
      graph_participant, topic,
        &DDS_DATAWRITER_QOS_DEFAULT, NULL, DDS_STATUS_MASK_NONE);
  }

  DDS_Topic * topic;
  DDS_DataReader * sub_reader;
  DDS_DataWriter * pub_writer;
  DDS_DataReader * client_reader;
  DDS_DataWriter * client_writer;
  DDS_DataReader * service_reader;
  DDS_DataWriter * service_writer;
};

TEST_F(TestGraphEndpoints, register_local_endpoints) {
  RTIROS2_GraphEndpointHandle sub_handle =
    RTIROS2_Graph_register_local_subscription(graph, node_handle, sub_reader);
  ASSERT_NE(sub_handle, RTIROS2_GraphNodeHandle_INVALID);
  DDS_DataReader *s_reader = RTIROS2_Graph_get_endpoint_reader(
    graph, node_handle, sub_handle);
  ASSERT_EQ(s_reader, sub_reader);
  DDS_DataWriter *s_writer = RTIROS2_Graph_get_endpoint_writer(
    graph, node_handle, sub_handle);
  ASSERT_EQ(s_writer, nullptr);
  RTIROS2_GraphEndpointType endp_type =
    RTIROS2_Graph_get_endpoint_type(graph, node_handle, sub_handle);
  ASSERT_EQ(endp_type, RTIROS2_GRAPH_ENDPOINT_SUBSCRIPTION);

  RTIROS2_GraphEndpointHandle pub_handle =
    RTIROS2_Graph_register_local_publisher(graph, node_handle, pub_writer);
  ASSERT_NE(pub_handle, RTIROS2_GraphNodeHandle_INVALID);
  ASSERT_NE(sub_handle, pub_handle);
  DDS_DataReader *p_reader = RTIROS2_Graph_get_endpoint_reader(
    graph, node_handle, pub_handle);
  ASSERT_EQ(p_reader, nullptr);
  DDS_DataWriter *p_writer = RTIROS2_Graph_get_endpoint_writer(
    graph, node_handle, pub_handle);
  ASSERT_EQ(p_writer, pub_writer);
  endp_type = RTIROS2_Graph_get_endpoint_type(graph, node_handle, pub_handle);
  ASSERT_EQ(endp_type, RTIROS2_GRAPH_ENDPOINT_PUBLISHER);

  RTIROS2_GraphEndpointHandle client_handle =
    RTIROS2_Graph_register_local_client(
      graph, node_handle, client_reader, client_writer);
  ASSERT_NE(client_handle, RTIROS2_GraphNodeHandle_INVALID);
  ASSERT_NE(sub_handle, client_handle);
  ASSERT_NE(pub_handle, client_handle);
  DDS_DataReader *c_reader = RTIROS2_Graph_get_endpoint_reader(
    graph, node_handle, client_handle);
  ASSERT_EQ(c_reader, client_reader);
  DDS_DataWriter *c_writer = RTIROS2_Graph_get_endpoint_writer(
    graph, node_handle, client_handle);
  ASSERT_EQ(c_writer, client_writer);
  endp_type = RTIROS2_Graph_get_endpoint_type(
    graph, node_handle, client_handle);
  ASSERT_EQ(endp_type, RTIROS2_GRAPH_ENDPOINT_CLIENT);

  RTIROS2_GraphEndpointHandle svc_handle =
    RTIROS2_Graph_register_local_service(
      graph, node_handle, service_reader, service_writer);
  ASSERT_NE(svc_handle, RTIROS2_GraphNodeHandle_INVALID);
  ASSERT_NE(sub_handle, svc_handle);
  ASSERT_NE(pub_handle, svc_handle);
  ASSERT_NE(client_handle, svc_handle);
  DDS_DataReader *svc_reader = RTIROS2_Graph_get_endpoint_reader(
    graph, node_handle, svc_handle);
  ASSERT_EQ(svc_reader, service_reader);
  DDS_DataWriter *svc_writer = RTIROS2_Graph_get_endpoint_writer(
    graph, node_handle, svc_handle);
  ASSERT_EQ(svc_writer, service_writer);
  endp_type = RTIROS2_Graph_get_endpoint_type(
    graph, node_handle, svc_handle);
  ASSERT_EQ(endp_type, RTIROS2_GRAPH_ENDPOINT_SERVICE);

  // Endpoints cannot be created with DDS endpoints already in use.
  RTIROS2_GraphEndpointHandle another_sub_handle =
    RTIROS2_Graph_register_local_subscription(graph, node_handle, sub_reader);
  ASSERT_EQ(another_sub_handle, RTIROS2_GraphNodeHandle_INVALID);

  RTIROS2_GraphEndpointHandle another_pub_handle =
    RTIROS2_Graph_register_local_publisher(graph, node_handle, pub_writer);
  ASSERT_EQ(another_pub_handle, RTIROS2_GraphNodeHandle_INVALID);

  RTIROS2_GraphEndpointHandle another_client_handle =
    RTIROS2_Graph_register_local_client(
      graph, node_handle, client_reader, client_writer);
  ASSERT_EQ(another_client_handle, RTIROS2_GraphNodeHandle_INVALID);

  RTIROS2_GraphEndpointHandle another_svc_handle =
    RTIROS2_Graph_register_local_service(
      graph, node_handle, service_reader, service_writer);
  ASSERT_EQ(another_svc_handle, RTIROS2_GraphNodeHandle_INVALID);

  // And this is true also when trying to register the DDS endpoint as another
  // type of ROS 2 endpoint:
  another_client_handle =
    RTIROS2_Graph_register_local_client(
      graph, node_handle, service_reader, service_writer);
  ASSERT_EQ(another_client_handle, RTIROS2_GraphNodeHandle_INVALID);

  another_svc_handle =
    RTIROS2_Graph_register_local_service(
      graph, node_handle, client_reader, client_writer);
  ASSERT_EQ(another_svc_handle, RTIROS2_GraphNodeHandle_INVALID);
}

TEST_F(TestGraphEndpoints, register_local_endpoints_bad_args) {
  // All DDS endpoints must be non-NULL.
  RTIROS2_GraphEndpointHandle sub_handle =
    RTIROS2_Graph_register_local_subscription(graph, node_handle, nullptr);
  ASSERT_EQ(sub_handle, RTIROS2_GraphNodeHandle_INVALID);

  RTIROS2_GraphEndpointHandle pub_handle =
    RTIROS2_Graph_register_local_publisher(graph, node_handle, nullptr);
  ASSERT_EQ(pub_handle, RTIROS2_GraphNodeHandle_INVALID);

  RTIROS2_GraphEndpointHandle client_handle = 
    RTIROS2_Graph_register_local_client(
      graph, node_handle, nullptr, nullptr);
  ASSERT_EQ(client_handle, RTIROS2_GraphNodeHandle_INVALID);

  client_handle = RTIROS2_Graph_register_local_client(
    graph, node_handle, client_reader, nullptr);
  ASSERT_EQ(client_handle, RTIROS2_GraphNodeHandle_INVALID);

  client_handle = RTIROS2_Graph_register_local_client(
    graph, node_handle, nullptr, client_writer);
  ASSERT_EQ(client_handle, RTIROS2_GraphNodeHandle_INVALID);

  RTIROS2_GraphEndpointHandle svc_handle =
    RTIROS2_Graph_register_local_service(
      graph, node_handle, nullptr, nullptr);
  ASSERT_EQ(svc_handle, RTIROS2_GraphNodeHandle_INVALID);

  svc_handle = RTIROS2_Graph_register_local_service(
    graph, node_handle, service_reader, nullptr);
  ASSERT_EQ(svc_handle, RTIROS2_GraphNodeHandle_INVALID);

  svc_handle = RTIROS2_Graph_register_local_service(
    graph, node_handle, nullptr, service_writer);
  ASSERT_EQ(svc_handle, RTIROS2_GraphNodeHandle_INVALID);

  // The node handle must be valid
  sub_handle = RTIROS2_Graph_register_local_subscription(
    graph, RTIROS2_GraphNodeHandle_INVALID, sub_reader);
  ASSERT_EQ(sub_handle, RTIROS2_GraphNodeHandle_INVALID);

  pub_handle = RTIROS2_Graph_register_local_publisher(
    graph, RTIROS2_GraphNodeHandle_INVALID, pub_writer);
  ASSERT_EQ(pub_handle, RTIROS2_GraphNodeHandle_INVALID);

  client_handle = RTIROS2_Graph_register_local_client(
      graph, RTIROS2_GraphNodeHandle_INVALID, client_reader, client_writer);
  ASSERT_EQ(client_handle, RTIROS2_GraphNodeHandle_INVALID);

  svc_handle = RTIROS2_Graph_register_local_service(
    graph, RTIROS2_GraphNodeHandle_INVALID, service_reader, service_writer);
  ASSERT_EQ(svc_handle, RTIROS2_GraphNodeHandle_INVALID);

  // The graph must be non-NULL
  sub_handle = RTIROS2_Graph_register_local_subscription(
    nullptr, node_handle, sub_reader);
  ASSERT_EQ(sub_handle, RTIROS2_GraphNodeHandle_INVALID);

  pub_handle = RTIROS2_Graph_register_local_publisher(
    nullptr, node_handle, pub_writer);
  ASSERT_EQ(pub_handle, RTIROS2_GraphNodeHandle_INVALID);

  client_handle = RTIROS2_Graph_register_local_client(
    nullptr, node_handle, client_reader, client_writer);
  ASSERT_EQ(client_handle, RTIROS2_GraphNodeHandle_INVALID);

  svc_handle = RTIROS2_Graph_register_local_service(
    nullptr, node_handle, service_reader, service_writer);
  ASSERT_EQ(svc_handle, RTIROS2_GraphNodeHandle_INVALID);
}


class TestGraphUpdates : public TestGraphEndpoints {
 protected:
  void SetUp() override {
    TestGraphEndpoints::SetUp();
    // Create a reader for the graph update topic
    DDS_Topic * graph_topic = RTIROS2_Graph_get_graph_topic(graph);
    DDS_DataReaderQos dr_qos = DDS_DataReaderQos_INITIALIZER;
    DDS_ReturnCode_t rc = RTIROS2_Graph_customize_datareader_qos(&dr_qos);
    ASSERT_EQ(rc, DDS_RETCODE_OK);

    DDS_SubscriberQos sub_qos = DDS_SubscriberQos_INITIALIZER;
    sub_qos.entity_factory.autoenable_created_entities = DDS_BOOLEAN_FALSE;

    graph_sub = DDS_DomainParticipant_create_subscriber(
      graph_participant, &sub_qos, NULL, DDS_STATUS_MASK_NONE);

    graph_reader = DDS_Subscriber_create_datareader(
      graph_sub, DDS_Topic_as_topicdescription(graph_topic),
      &dr_qos, NULL, DDS_STATUS_MASK_NONE);
    ASSERT_NE(graph_reader, nullptr);
  }

  void TearDown() override {
    DDS_ReturnCode_t rc = DDS_Subscriber_delete_datareader(
      graph_sub, graph_reader);
    ASSERT_EQ(rc, DDS_RETCODE_OK);
    rc = DDS_DomainParticipant_delete_subscriber(graph_participant, graph_sub);
    ASSERT_EQ(rc, DDS_RETCODE_OK);
    TestGraphEndpoints::TearDown();
  }

  DDS_DataReader * graph_reader;
  DDS_Subscriber * graph_sub;
};

TEST_F(TestGraphUpdates, publish_updates) {
  RTIROS2_GraphEndpointHandle sub_handle =
    RTIROS2_Graph_register_local_subscription(graph, node_handle, sub_reader);
  ASSERT_NE(sub_handle, RTIROS2_GraphNodeHandle_INVALID);

  RTIROS2_GraphEndpointHandle pub_handle =
    RTIROS2_Graph_register_local_publisher(graph, node_handle, pub_writer);
  ASSERT_NE(pub_handle, RTIROS2_GraphNodeHandle_INVALID);

  RTIROS2_GraphEndpointHandle client_handle =
    RTIROS2_Graph_register_local_client(
      graph, node_handle, client_reader, client_writer);
  ASSERT_NE(client_handle, RTIROS2_GraphNodeHandle_INVALID);

  RTIROS2_GraphEndpointHandle svc_handle =
    RTIROS2_Graph_register_local_service(
      graph, node_handle, service_reader, service_writer);
  ASSERT_NE(svc_handle, RTIROS2_GraphNodeHandle_INVALID);

  RTIROS2_ParticipantEntitiesInfoDataReader * g_reader =
    RTIROS2_ParticipantEntitiesInfoDataReader_narrow(graph_reader);

  // Sleep for a little bit to allow the graph to publish its latest update
  std::this_thread::sleep_for(100ms);

  // Enable the DataReader now, and wait for it to receive data.
  DDS_Entity * r_entity = DDS_DataReader_as_entity(graph_reader);

  DDS_ReturnCode_t rc = DDS_Entity_enable(r_entity);
  ASSERT_EQ(rc, DDS_RETCODE_OK);

  DDS_StatusMask status_changes = DDS_Entity_get_status_changes(r_entity);
  do {
    if (!(status_changes & DDS_DATA_AVAILABLE_STATUS))
    {
      std::this_thread::sleep_for(100ms);
    }
    status_changes = DDS_Entity_get_status_changes(r_entity);
  } while (!(status_changes & DDS_DATA_AVAILABLE_STATUS));

  DDS_SampleInfoSeq info_seq = DDS_SEQUENCE_INITIALIZER;
  RTIROS2_ParticipantEntitiesInfoSeq data_seq = DDS_SEQUENCE_INITIALIZER;

  rc = RTIROS2_ParticipantEntitiesInfoDataReader_take(
    g_reader,
    &data_seq,
    &info_seq,
    DDS_LENGTH_UNLIMITED,
    DDS_ANY_VIEW_STATE,
    DDS_ANY_SAMPLE_STATE,
    DDS_ANY_INSTANCE_STATE);
  ASSERT_EQ(rc, DDS_RETCODE_OK);

  DDS_Long data_len = RTIROS2_ParticipantEntitiesInfoSeq_get_length(&data_seq);
  ASSERT_EQ(data_len, 1);

  RTIROS2_ParticipantEntitiesInfo * pinfo =
    RTIROS2_ParticipantEntitiesInfoSeq_get_reference(&data_seq, 0);
  ASSERT_NE(pinfo, nullptr);

  RTIROS2_Gid graph_participant_gid;
  rc = RTIROS2_Graph_compute_participant_gid(
    graph_participant, &graph_participant_gid);
  ASSERT_EQ(rc, DDS_RETCODE_OK);

  int cmp_res =
    memcmp(pinfo->gid.data, graph_participant_gid.data, RTIROS2_GID_LENGTH);
  ASSERT_EQ(cmp_res, 0);

  DDS_Long nodes_len =
    RTIROS2_NodeEntitiesInfoSeq_get_length(&pinfo->node_entities_info_seq);
  ASSERT_EQ(nodes_len, 1);

  RTIROS2_NodeEntitiesInfo * ninfo =
    RTIROS2_NodeEntitiesInfoSeq_get_reference(
      &pinfo->node_entities_info_seq, 0);
  ASSERT_NE(ninfo, nullptr);

  ASSERT_EQ(strcmp(ninfo->node_name, "foo"), 0);
  ASSERT_EQ(strcmp(ninfo->node_namespace, ""), 0);

  DDS_Long readers_len = RTIROS2_GidSeq_get_length(&ninfo->reader_gid_seq);
  ASSERT_EQ(readers_len, 3);
  DDS_Long writers_len = RTIROS2_GidSeq_get_length(&ninfo->writer_gid_seq);
  ASSERT_EQ(writers_len, 3);

  rc = RTIROS2_ParticipantEntitiesInfoDataReader_return_loan(
    g_reader, &data_seq, &info_seq);
  ASSERT_EQ(rc, DDS_RETCODE_OK);
}