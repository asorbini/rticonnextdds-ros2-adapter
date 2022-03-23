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

#include <rti/request/Requester.hpp>

#include "rticonnextdds_ros2_adapter/rticonnextdds_ros2_adapter_cpp.hpp"

#include "Parameters.hpp"
#include "ParametersPlugin.hpp"

using namespace std::chrono_literals;

int main(int argc, char **argv)
{
  (void)argc; (void)argv;
  static const char * const topic_name_req = "rq/talker/list_parametersRequest";
  static const char * const topic_name_rep = "rr/talker/list_parametersReply";

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

  rti::request::RequesterParams req_params(participant);
  req_params.request_topic_name(topic_name_req);
  req_params.reply_topic_name(topic_name_rep);
  req_params.datawriter_qos(dw_qos);
  req_params.datareader_qos(dr_qos);
  req_params.publisher(publisher);
  req_params.subscriber(subscriber);

  rti::request::Requester<
    rcl_interfaces::srv::dds_::ListParameters_Request_,
    rcl_interfaces::srv::dds_::ListParameters_Response_>
  requester(req_params);

  rcl_interfaces::srv::dds_::ListParameters_Request_ request;
  request.depth(0);

  rti::ros2::GraphProperties g_props;
  g_props.graph_participant = participant;
  rti::ros2::Graph graph(g_props);

  auto node_handle = graph.register_local_node("params_client");
  assert(rti::ros2::GraphNodeHandle_INVALID != node_handle);

  graph.inspect_local_node(node_handle);

  while (true)
  {
    std::cout << "Sending request..." << std::endl;
    requester.send_request(request);

    auto replies = requester.receive_replies(dds::core::Duration::from_secs(10));
    if (replies.length() == 0) {
        std::cout << "No reply received\n";
    }
    for (const auto& reply : replies) {
      if (reply.info().valid()) {
          std::cout << "Received reply: " << std::endl;
          for (auto & name : reply.data().result().names())
          {
            std::cout << "  - " << name << std::endl;
          }
      } else {
          std::cout << "Received invalid reply\n";
      }
    }

    std::this_thread::sleep_for(5s);
  }

  return 0;
}