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
#include <memory>

#include <gtest/gtest.h>

#include "rticonnextdds_ros2_adapter/rticonnextdds_ros2_adapter_cpp.hpp"
#include "rticonnextdds_ros2_adapter_test/rticonnextdds_ros2_adapter_test_model.hpp"
#include "rticonnextdds_ros2_adapter_test/rticonnextdds_ros2_adapter_test_modelPlugin.hpp"

using namespace std::chrono_literals;
using namespace rti::ros2;

class TestGraphCpp : public ::testing::Test {
 protected:
  void SetUp() override {
    auto qos_provider = dds::core::QosProvider::Default();
    dp_qos = qos_provider.participant_qos();
    rti::core::policy::Database db_policy;
    dp_qos >> db_policy;
    db_policy.shutdown_cleanup_period(dds::core::Duration(0, 10000000));
    dp_qos << db_policy;

    graph_participant = dds::domain::DomainParticipant(0, dp_qos);
    ASSERT_NE(graph_participant, nullptr);

    another_participant = dds::domain::DomainParticipant(0, dp_qos);;
    ASSERT_NE(another_participant, nullptr);
  }

  void TearDown() override {
    graph_participant->close();    
    another_participant->close();
    dds::domain::DomainParticipant::finalize_participant_factory();
  }

  dds::domain::DomainParticipant graph_participant{nullptr};
  dds::domain::DomainParticipant another_participant{nullptr};
  dds::domain::qos::DomainParticipantQos dp_qos;
};

TEST_F(TestGraphCpp, new_graph) {
  GraphProperties props;
  props.graph_participant = graph_participant;

  Graph graph(props);

  auto g_participant = graph.graph_participant();
  ASSERT_EQ(g_participant, graph_participant);

  auto g_publisher = graph.graph_publisher();
  ASSERT_NE(g_publisher, nullptr);

  auto g_topic = graph.graph_topic();
  ASSERT_NE(g_topic, nullptr);

  auto g_writer = graph.graph_writer();
  ASSERT_NE(g_writer, nullptr);

  // We can create more graphs if needed.
  Graph another_graph = Graph(props);

  auto ag_participant = another_graph.graph_participant();
  ASSERT_EQ(ag_participant, g_participant);

  auto ag_publisher = another_graph.graph_publisher();
  ASSERT_NE(ag_publisher, nullptr);
  ASSERT_NE(ag_publisher, g_publisher);

  auto ag_topic = another_graph.graph_topic();
  ASSERT_EQ(ag_topic, g_topic);

  auto ag_writer = another_graph.graph_writer();
  ASSERT_NE(ag_writer, nullptr);
  ASSERT_NE(ag_writer, g_writer);
}

TEST_F(TestGraphCpp, new_graph_bad_arguments) {
  GraphProperties props;

  // properties must include at least a DomainParticipant.
  try {
    Graph graph(props);
    FAIL() << "Expected an exception";
  } 
  catch (dds::core::InvalidArgumentError & e)
  {
    EXPECT_EQ(e.what(), std::string("no domain participant"));
  }
  catch (...) {
    FAIL() << "Unexpected exception thrown";
  }
}

TEST_F(TestGraphCpp, register_local_node) {
  GraphProperties props;
  props.graph_participant = graph_participant;
  
  Graph graph(props);

  // Namespace might be empty
  GraphNodeHandle node_handle_1 = graph.register_local_node(
    "foo", "", another_participant);
  ASSERT_NE(node_handle_1, GraphNodeHandle_INVALID);

  auto node_participant = graph.get_node_participant(node_handle_1);
  ASSERT_EQ(node_participant, another_participant);

  // or a non-empty namespace might be specified
  GraphNodeHandle node_handle_2 =
    graph.register_local_node("foo", "bar", graph_participant);
  ASSERT_NE(node_handle_2, GraphNodeHandle_INVALID);
  node_participant = graph.get_node_participant(node_handle_2);
  ASSERT_EQ(node_participant, graph_participant);

  // The participant is optional, in which case the graph's own participant is
  // used.
  GraphNodeHandle node_handle_3 = graph.register_local_node("foo1");
  ASSERT_NE(node_handle_3, GraphNodeHandle_INVALID);
  node_participant = graph.get_node_participant(node_handle_3);
  ASSERT_EQ(node_participant, graph_participant);

  // We cannot register a node that has already been registered
  GraphNodeHandle node_handle_4 =
    graph.register_local_node("foo", "", another_participant);
  ASSERT_EQ(node_handle_4, GraphNodeHandle_INVALID);

  node_handle_4 = graph.register_local_node("foo", "bar", graph_participant);
  ASSERT_EQ(node_handle_4, GraphNodeHandle_INVALID);

  node_handle_4 = graph.register_local_node("foo1");
  ASSERT_EQ(node_handle_4, GraphNodeHandle_INVALID);
}

TEST_F(TestGraphCpp, register_local_node_bad_args) {
  GraphProperties props;
  props.graph_participant = graph_participant;
  Graph graph(props);
  
  // Name must be non-empty
  GraphNodeHandle node_handle = graph.register_local_node("");
  ASSERT_EQ(node_handle, GraphNodeHandle_INVALID);
}

class TestGraphCppNode : public TestGraphCpp {
 protected:
  void SetUp() override {
    TestGraphCpp::SetUp();
    GraphProperties props;
    props.graph_participant = graph_participant;
    graph = std::make_unique<Graph>(props);
    ASSERT_NE(graph, nullptr);

    node_handle = graph->register_local_node("foo");
    ASSERT_NE(node_handle, GraphNodeHandle_INVALID);
  }

  void TearDown() override {
    graph.reset();
    TestGraphCpp::TearDown();
  }

  std::unique_ptr<Graph> graph;
  GraphNodeHandle node_handle;
};

class TestGraphCppEndpoints : public TestGraphCppNode {
 protected:
  void SetUp() override {
    TestGraphCppNode::SetUp();

    subscriber = dds::sub::Subscriber(graph_participant);
    ASSERT_NE(subscriber, nullptr);

    publisher = dds::pub::Publisher(graph_participant);
    ASSERT_NE(publisher, nullptr);

    topic = dds::topic::Topic<RTIROS2::String>(
      graph_participant, "rt/foo", "String");
    ASSERT_NE(topic, nullptr);

    topic_req = dds::topic::Topic<RTIROS2::String>(
      graph_participant, "rq/foo/test_serviceRequest", "String");
    ASSERT_NE(topic_req, nullptr);

    topic_rep = dds::topic::Topic<RTIROS2::String>(
      graph_participant, "rr/foo/test_serviceReply", "String");
    ASSERT_NE(topic_rep, nullptr);

    sub_reader = create_reader(topic);
    ASSERT_NE(sub_reader, nullptr);
    pub_writer = create_writer(topic);
    ASSERT_NE(pub_writer, nullptr);

    client_reader = create_reader(topic_rep);
    ASSERT_NE(client_reader, nullptr);
    client_writer = create_writer(topic_req);
    ASSERT_NE(client_writer, nullptr);

    service_reader = create_reader(topic_req);
    ASSERT_NE(service_reader, nullptr);
    service_writer = create_writer(topic_rep);
    ASSERT_NE(service_writer, nullptr);
  }

  void TearDown() override {
    sub_reader->close();
    pub_writer->close();
    client_reader->close();
    client_writer->close();
    service_reader->close();
    service_writer->close();
    topic->close();
    subscriber->close();
    publisher->close();
    TestGraphCppNode::TearDown();
  }

  dds::sub::DataReader<RTIROS2::String>
  create_reader(dds::topic::Topic<RTIROS2::String> & topic)
  {
    return dds::sub::DataReader<RTIROS2::String>(subscriber, topic);
  }

  dds::pub::DataWriter<RTIROS2::String>
  create_writer(dds::topic::Topic<RTIROS2::String> & topic)
  {
    return dds::pub::DataWriter<RTIROS2::String>(publisher, topic);
  }

  dds::topic::Topic<RTIROS2::String> topic{nullptr};
  dds::topic::Topic<RTIROS2::String> topic_req{nullptr};
  dds::topic::Topic<RTIROS2::String> topic_rep{nullptr};
  dds::sub::Subscriber subscriber{nullptr};
  dds::pub::Publisher publisher{nullptr};
  dds::sub::DataReader<RTIROS2::String> sub_reader{nullptr};
  dds::pub::DataWriter<RTIROS2::String> pub_writer{nullptr};
  dds::sub::DataReader<RTIROS2::String> client_reader{nullptr};
  dds::pub::DataWriter<RTIROS2::String> client_writer{nullptr};
  dds::sub::DataReader<RTIROS2::String> service_reader{nullptr};
  dds::pub::DataWriter<RTIROS2::String> service_writer{nullptr};
};

TEST_F(TestGraphCppEndpoints, register_local_endpoints) {
  // dds::sub::AnyDataReader sub_reader_a(sub_reader);
  GraphEndpointHandle sub_handle =
    graph->register_local_subscription(node_handle, sub_reader);
  ASSERT_NE(sub_handle, GraphNodeHandle_INVALID);
  auto s_reader_o = graph->get_endpoint_reader(node_handle, sub_handle);
  ASSERT_TRUE(s_reader_o.is_set());
  auto s_reader = s_reader_o.get();
  ASSERT_EQ(s_reader, sub_reader);
  auto s_writer_o = graph->get_endpoint_writer(node_handle, sub_handle);
  ASSERT_FALSE(s_writer_o.is_set());
  auto endp_type = graph->get_endpoint_type(node_handle, sub_handle);
  ASSERT_EQ(endp_type, GraphEndpointType::Subscription);

  GraphEndpointHandle pub_handle =
    graph->register_local_publisher(node_handle, pub_writer);
  ASSERT_NE(pub_handle, GraphNodeHandle_INVALID);
  ASSERT_NE(sub_handle, pub_handle);
  auto p_reader_o = graph->get_endpoint_reader(node_handle, pub_handle);
  ASSERT_FALSE(p_reader_o.is_set());
  auto p_writer_o = graph->get_endpoint_writer(node_handle, pub_handle);
  ASSERT_TRUE(p_writer_o.is_set());
  auto p_writer = p_writer_o.get();
  ASSERT_EQ(p_writer, pub_writer);
  endp_type = graph->get_endpoint_type(node_handle, pub_handle);
  ASSERT_EQ(endp_type, GraphEndpointType::Publisher);

  GraphEndpointHandle client_handle =
    graph->register_local_client(
      node_handle, client_reader, client_writer);
  ASSERT_NE(client_handle, GraphNodeHandle_INVALID);
  ASSERT_NE(sub_handle, client_handle);
  ASSERT_NE(pub_handle, client_handle);
  auto c_reader_o = graph->get_endpoint_reader(node_handle, client_handle);
  ASSERT_TRUE(c_reader_o.is_set());
  auto c_reader = c_reader_o.get();
  ASSERT_EQ(c_reader, client_reader);
  auto c_writer_o = graph->get_endpoint_writer(node_handle, client_handle);
  ASSERT_TRUE(c_writer_o.is_set());
  auto c_writer = c_writer_o.get();
  ASSERT_EQ(c_writer, client_writer);
  endp_type = graph->get_endpoint_type(node_handle, client_handle);
  ASSERT_EQ(endp_type, GraphEndpointType::Client);

  GraphEndpointHandle svc_handle =
    graph->register_local_service(node_handle, service_reader, service_writer);
  ASSERT_NE(svc_handle, GraphNodeHandle_INVALID);
  ASSERT_NE(sub_handle, svc_handle);
  ASSERT_NE(pub_handle, svc_handle);
  ASSERT_NE(client_handle, svc_handle);
  auto svc_reader_o = graph->get_endpoint_reader(node_handle, svc_handle);
  ASSERT_TRUE(svc_reader_o.is_set());
  auto svc_reader = svc_reader_o.get();
  ASSERT_EQ(svc_reader, service_reader);
  auto svc_writer_o = graph->get_endpoint_writer(node_handle, svc_handle);
  ASSERT_TRUE(svc_writer_o.is_set());
  auto svc_writer = svc_writer_o.get();
  ASSERT_EQ(svc_writer, service_writer);
  endp_type = graph->get_endpoint_type(node_handle, svc_handle);
  ASSERT_EQ(endp_type, GraphEndpointType::Service);

  // Endpoints cannot be created with DDS endpoints already in use.
  GraphEndpointHandle another_sub_handle =
    graph->register_local_subscription(node_handle, sub_reader);
  ASSERT_EQ(another_sub_handle, GraphNodeHandle_INVALID);

  RTIROS2_GraphEndpointHandle another_pub_handle =
    graph->register_local_publisher(node_handle, pub_writer);
  ASSERT_EQ(another_pub_handle, GraphNodeHandle_INVALID);

  RTIROS2_GraphEndpointHandle another_client_handle =
    graph->register_local_client(node_handle, client_reader, client_writer);
  ASSERT_EQ(another_client_handle, GraphNodeHandle_INVALID);

  RTIROS2_GraphEndpointHandle another_svc_handle =
    graph->register_local_service(node_handle, service_reader, service_writer);
  ASSERT_EQ(another_svc_handle, GraphNodeHandle_INVALID);

  // And this is true also when trying to register the DDS endpoint as another
  // type of ROS 2 endpoint:
  another_client_handle =
    graph->register_local_client(node_handle, service_reader, service_writer);
  ASSERT_EQ(another_client_handle, GraphNodeHandle_INVALID);

  another_svc_handle =
    graph->register_local_service(node_handle, client_reader, client_writer);
  ASSERT_EQ(another_svc_handle, GraphNodeHandle_INVALID);
}

TEST_F(TestGraphCppEndpoints, register_local_endpoints_bad_args) {
  // The node handle must be valid
  GraphEndpointHandle sub_handle =
    graph->register_local_subscription(GraphNodeHandle_INVALID, sub_reader);
  ASSERT_EQ(sub_handle, GraphNodeHandle_INVALID);

  GraphEndpointHandle pub_handle =
    graph->register_local_publisher(GraphNodeHandle_INVALID, pub_writer);
  ASSERT_EQ(pub_handle, GraphNodeHandle_INVALID);

  GraphEndpointHandle client_handle = graph->register_local_client(
    GraphNodeHandle_INVALID, client_reader, client_writer);
  ASSERT_EQ(client_handle, GraphNodeHandle_INVALID);

  GraphEndpointHandle svc_handle = graph->register_local_service(
    GraphNodeHandle_INVALID, service_reader, service_writer);
  ASSERT_EQ(svc_handle, GraphNodeHandle_INVALID);
}


class TestGraphCppUpdates : public TestGraphCppEndpoints {
 protected:
  void SetUp() override {
    TestGraphCppEndpoints::SetUp();
    // Create a reader for the graph update topic
    dds::topic::Topic<RTIROS2::ParticipantEntitiesInfo> graph_topic =
      graph->graph_topic();

    dds::sub::qos::SubscriberQos sub_qos = graph_participant->default_subscriber_qos();
    dds::core::policy::EntityFactory factory_qos;
    // factory_qos << sub_qos;
    factory_qos.autoenable_created_entities(DDS_BOOLEAN_FALSE);
    sub_qos << factory_qos;

    graph_subscriber = dds::sub::Subscriber(graph_participant, sub_qos);
    ASSERT_NE(graph_subscriber, nullptr);

    dds::sub::qos::DataReaderQos dr_qos =
      graph_subscriber->default_datareader_qos();
    rti::ros2::graph::customize_datareader_qos(dr_qos);
    graph_reader = dds::sub::DataReader<RTIROS2::ParticipantEntitiesInfo>(
      graph_subscriber, graph_topic, dr_qos);
    ASSERT_NE(graph_reader, nullptr);
  }

  void TearDown() override {
    graph_reader->close();
    graph_subscriber->close();
    TestGraphCppEndpoints::TearDown();
  }
  dds::sub::Subscriber graph_subscriber{nullptr};
  dds::sub::DataReader<RTIROS2::ParticipantEntitiesInfo> graph_reader{nullptr};
};

TEST_F(TestGraphCppUpdates, publish_updates) {
  GraphEndpointHandle sub_handle =
    graph->register_local_subscription(node_handle, sub_reader);
  ASSERT_NE(sub_handle, GraphNodeHandle_INVALID);

  GraphEndpointHandle pub_handle =
    graph->register_local_publisher(node_handle, pub_writer);
  ASSERT_NE(pub_handle, GraphNodeHandle_INVALID);

  GraphEndpointHandle client_handle =
    graph->register_local_client(
      node_handle, client_reader, client_writer);
  ASSERT_NE(client_handle, GraphNodeHandle_INVALID);

  GraphEndpointHandle svc_handle =
    graph->register_local_service(node_handle, service_reader, service_writer);
  ASSERT_NE(svc_handle, GraphNodeHandle_INVALID);

  // Sleep for a little bit to allow the graph to publish its latest update
  std::this_thread::sleep_for(100ms);

  // Enable the DataReader now, and wait for it to receive data.
  graph_reader->enable();

   dds::core::status::StatusMask status_changes =
    graph_reader->status_changes();

  do {
    if (!(status_changes & dds::core::status::StatusMask::data_available()).any())
    {
      std::this_thread::sleep_for(100ms);
    }
    status_changes = graph_reader->status_changes();
  } while (!(status_changes & dds::core::status::StatusMask::data_available()).any());

  auto samples = graph_reader->take();
  ASSERT_EQ(samples.length(), 1);

  ASSERT_TRUE(samples[0].info().valid());

  auto data = samples[0].data();

  RTIROS2::Gid graph_participant_gid;
  rti::ros2::graph::compute_participant_gid(
    graph_participant, graph_participant_gid);

  int cmp_res =
    memcmp(
      &(data.gid().data()[0]),
      &(graph_participant_gid.data()[0]),
      RTIROS2::GID_LENGTH);
  ASSERT_EQ(cmp_res, 0);

  ASSERT_EQ(data.node_entities_info_seq().size(), 1);

  auto ndata = data.node_entities_info_seq()[0];

  ASSERT_EQ(ndata.node_name(), std::string("foo"));
  ASSERT_EQ(ndata.node_namespace(), std::string("/"));

  ASSERT_EQ(ndata.reader_gid_seq().size(), 3);
  ASSERT_EQ(ndata.writer_gid_seq().size(), 3);
}