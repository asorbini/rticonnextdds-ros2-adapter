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

#include "rticonnextdds_ros2_adapter/rticonnextdds_ros2_adapter_cpp.hpp"

#include "String.hpp"
#include "StringPlugin.hpp"

int main(int argc, char **argv)
{
  (void)argc; (void)argv;
  static const char * const type_name = "std_msgs::msg::dds_::String";
  static const char * const topic_name_in = "rt/chatter";
  static const char * const topic_name_out = "rt/chatter_dup";

  dds::domain::DomainParticipant my_participant(0);
  assert(nullptr != my_participant);

  dds::sub::Subscriber my_subscriber = dds::sub::Subscriber(my_participant);
  assert(nullptr != my_subscriber);

  dds::pub::Publisher my_publisher = dds::pub::Publisher(my_participant);
  assert(nullptr != my_publisher);

  dds::topic::Topic<String> my_topic_in(
    my_participant, topic_name_in, type_name);

  dds::topic::Topic<String> my_topic_out(
    my_participant, topic_name_out, type_name);

  dds::pub::qos::DataWriterQos dw_qos;
  my_publisher->default_writer_qos(dw_qos);
  rti::core::policy::Property dw_props;
  dw_props.set(
    {
      "dds.data_writer.history.memory_manager.fast_pool.pool_buffer_max_size",
      "1024"
    }, false);
  dw_qos << dw_props;

  dds::sub::qos::DataReaderQos dr_qos;
    my_subscriber->default_datareader_qos(dr_qos);
  rti::core::policy::Property dr_props;
  dr_props.set(
    {
      "dds.data_reader.history.memory_manager.fast_pool.pool_buffer_max_size",
      "1024"
    }, false);
  dr_qos << dr_props;

  dds::sub::DataReader<String> my_reader(my_subscriber, my_topic_in, dr_qos);
  assert(nullptr != my_reader);

  dds::pub::DataWriter<String> my_writer(my_publisher, my_topic_out, dw_qos);
  assert(nullptr != my_writer);

  dds::core::cond::WaitSet waitset;

  dds::core::cond::StatusCondition my_reader_cond(my_reader);
  my_reader_cond.enabled_statuses(
    dds::core::status::StatusMask::data_available() |
    dds::core::status::StatusMask::subscription_matched());
  dds::core::cond::StatusCondition my_writer_cond(my_writer);
  my_writer_cond.enabled_statuses(
    dds::core::status::StatusMask::publication_matched());

  dds::core::cond::GuardCondition exit_cond;

  waitset += my_reader_cond;
  waitset += my_writer_cond;
  waitset += exit_cond;

  rti::ros2::GraphProperties g_props;
  g_props.graph_participant = my_participant;
  rti::ros2::Graph graph(g_props);

  auto node_handle = graph.register_local_node("dds_proxy");
  assert(rti::ros2::GraphNodeHandle_INVALID != node_handle);

  auto endp_handle = graph.register_local_subscription(node_handle, my_reader);
  assert(rti::ros2::GraphEndpointHandle_INVALID != endp_handle);

  endp_handle = graph.register_local_publisher(node_handle, my_writer);
  assert(rti::ros2::GraphEndpointHandle_INVALID != endp_handle);

  bool active = true;
  while (active)
  {
    auto active_conditions = waitset.wait(dds::core::Duration::infinite());
    for (uint32_t i = 0; i < active_conditions.size(); i++) {
      if (active_conditions[i] == exit_cond)
      {
        active = false;
      }
      else if (active_conditions[i] == my_reader_cond)
      {
        auto status_changes = my_reader->status_changes();
        if ((status_changes &
          dds::core::status::StatusMask::subscription_matched()).any())
        {
          auto sub_match_status = my_reader->subscription_matched_status();
          printf("DataReader match status changed:\n");
          printf("  total: %d (%d), current: %d (%d)\n",
            sub_match_status.total_count(),
            sub_match_status.total_count_change(),
            sub_match_status.current_count(),
            sub_match_status.current_count_change());
        }
        else if ((status_changes &
          dds::core::status::StatusMask::data_available()).any())
        {
          auto samples = my_reader->take();
          for (size_t j = 0; j < samples.length(); j++)
          {
            if (!samples[j].info().valid())
            {
              continue;
            }
            auto data = samples[j].data();
            printf("Forwarding data: '%s'\n", data.data().c_str());
            my_writer.write(data);
          }
        }
      }
      else if (active_conditions[i] == my_writer_cond)
      {
        auto status_changes = my_writer->status_changes();
        if ((status_changes &
          dds::core::status::StatusMask::publication_matched()).any())
        {
          auto pub_match_status = my_writer->publication_matched_status();
          printf("DataWriter match status changed:\n");
          printf("  total: %d (%d), current: %d (%d)\n",
            pub_match_status.total_count(),
            pub_match_status.total_count_change(),
            pub_match_status.current_count(),
            pub_match_status.current_count_change());
        }
      }
    }
  }

  return 0;
}