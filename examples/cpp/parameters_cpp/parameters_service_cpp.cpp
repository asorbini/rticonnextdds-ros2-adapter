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

#include <assert.h>
#include <thread>
#include <chrono>

#include <rti/request/Replier.hpp>

#include "rticonnextdds_ros2_adapter/rticonnextdds_ros2_adapter_cpp.hpp"

#include "Parameters.hpp"
#include "ParametersPlugin.hpp"

using namespace std::chrono_literals;

int main(int argc, char **argv)
{
  (void)argc; (void)argv;
  static const char * const topic_name_req = "rq/foo/list_parametersRequest";
  static const char * const topic_name_rep = "rr/foo/list_parametersReply";

  dds::domain::DomainParticipant participant(0);
  assert(nullptr != participant);

  dds::sub::Subscriber subscriber = dds::sub::Subscriber(participant);
  assert(nullptr != subscriber);

  dds::pub::Publisher publisher = dds::pub::Publisher(participant);
  assert(nullptr != publisher);

  dds::pub::qos::DataWriterQos dw_qos;
  publisher->default_writer_qos(dw_qos);
  rti::core::policy::Property dw_props;
  dw_props.set(
    {
      "dds.data_writer.history.memory_manager.fast_pool.pool_buffer_max_size",
      "1024"
    }, false);
  dw_qos << dw_props;

  dds::sub::qos::DataReaderQos dr_qos;
  subscriber->default_datareader_qos(dr_qos);
  rti::core::policy::Property dr_props;
  dr_props.set(
    {
      "dds.data_reader.history.memory_manager.fast_pool.pool_buffer_max_size",
      "1024"
    }, false);
  dr_qos << dr_props;

  rti::request::ReplierParams rep_params(participant);
  rep_params.request_topic_name(topic_name_req);
  rep_params.reply_topic_name(topic_name_rep);
  rep_params.datawriter_qos(dw_qos);
  rep_params.datareader_qos(dr_qos);
  rep_params.publisher(publisher);
  rep_params.subscriber(subscriber);

  rti::request::Replier<
    rcl_interfaces::srv::dds_::ListParameters_Request_,
    rcl_interfaces::srv::dds_::ListParameters_Response_>
  replier(rep_params);

  rcl_interfaces::srv::dds_::ListParameters_Response_ response;
  response.result().names().resize(3);
  response.result().names()[0] = "foo";
  response.result().names()[1] = "bar";
  response.result().names()[2] = "baz";

  // Create a ROS 2 graph object, then register a ROS 2 node
  // and associate it with the DomainParticipant. 
  // Let the graph inspect the participant and automatically
  // detect endpoints which follow the ROS 2 naming conventions.
  rti::ros2::GraphProperties g_props;
  g_props.graph_participant = participant;
  rti::ros2::Graph graph(g_props);

  auto node_handle = graph.register_local_node("params_service");
  assert(rti::ros2::GraphNodeHandle_INVALID != node_handle);

  graph.inspect_local_node(node_handle);

  while (true)
  {
    std::cout << "Waiting for requests..." << std::endl;
    dds::sub::LoanedSamples<rcl_interfaces::srv::dds_::ListParameters_Request_>
      requests = replier.receive_requests(dds::core::Duration::from_secs(10));

    for (const auto& request : requests) {
      if (!request.info().valid()) {
        std::cout << "Received invalid request\n";
        continue;
      }

      std::cout << "Received request!" << std::endl;
      replier.send_reply(response, request.info());
    }
  }

  return 0;
}