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

#include "rticonnextdds_ros2_adapter/rticonnextdds_ros2_adapter_c.h"

#include "StringSupport.h"

int main(int argc, char **argv)
{
  (void)argc; (void)argv;
  DDS_DomainParticipantFactory * factory = NULL;
  DDS_DomainParticipant * my_participant = NULL;
  DDS_Subscriber * my_subscriber = NULL;
  DDS_Publisher * my_publisher = NULL;
  DDS_Topic * my_topic_in = NULL;
  DDS_Topic * my_topic_out = NULL;
  DDS_DataReader * my_reader = NULL;
  DDS_DataWriter * my_writer = NULL;
  struct DDS_DataReaderQos dr_qos = DDS_DataReaderQos_INITIALIZER;
  struct DDS_DataWriterQos dw_qos = DDS_DataWriterQos_INITIALIZER;
  struct RTIROS2_GraphProperties g_props = RTIROS2_GraphProperties_INITIALIZER;
  RTIROS2_Graph * graph = NULL;
  DDS_WaitSet * waitset = NULL;
  struct DDS_Duration_t wait_timeout = DDS_DURATION_INFINITE;
  struct DDS_ConditionSeq active_conditions = DDS_SEQUENCE_INITIALIZER;
  DDS_Condition * active_cond = NULL;
  DDS_Long active_conditions_len = 0;
  DDS_Long data_len = 0;
  DDS_Long i = 0;
  DDS_Long j = 0;
  DDS_StatusCondition * my_reader_cond = NULL;
  DDS_StatusCondition * my_writer_cond = NULL;
  DDS_GuardCondition * exit_cond = NULL;
  DDS_Boolean active = DDS_BOOLEAN_TRUE;
  DDS_ReturnCode_t rc = DDS_RETCODE_ERROR;
  DDS_StatusMask status_changes = DDS_STATUS_MASK_NONE;
  struct DDS_PublicationMatchedStatus pub_match_status =
    DDS_PublicationMatchedStatus_INITIALIZER;
  struct DDS_SubscriptionMatchedStatus sub_match_status =
    DDS_SubscriptionMatchedStatus_INITIALIZER;
  struct StringSeq data_seq = DDS_SEQUENCE_INITIALIZER;
  struct DDS_SampleInfoSeq info_seq = DDS_SEQUENCE_INITIALIZER;
  StringDataReader * treader = NULL;
  StringDataWriter * twriter = NULL;
  String * data = NULL;
  struct DDS_SampleInfo * info = NULL;
  RTIROS2_GraphNodeHandle node_handle = RTIROS2_GraphNodeHandle_INVALID;
  RTIROS2_GraphEndpointHandle endp_handle = RTIROS2_GraphEndpointHandle_INVALID;
  static const char * const type_name = "std_msgs::msg::dds_::String";
  static const char * const topic_name_in = "rt/chatter";
  static const char * const topic_name_out = "rt/chatter_dup";

  factory = DDS_DomainParticipantFactory_get_instance();
  assert(NULL != factory);

  my_participant = DDS_DomainParticipantFactory_create_participant(
    factory, 0, &DDS_PARTICIPANT_QOS_DEFAULT, NULL, DDS_STATUS_MASK_NONE);
  assert(NULL != my_participant);

  my_subscriber = DDS_DomainParticipant_get_builtin_subscriber(my_participant);
  assert(NULL != my_subscriber);

  my_publisher = DDS_DomainParticipant_get_builtin_publisher(my_participant);
  assert(NULL != my_publisher);

  rc = StringTypeSupport_register_type(my_participant, type_name);
  assert(DDS_RETCODE_OK == rc);

  my_topic_in = DDS_DomainParticipant_create_topic(
    my_participant, topic_name_in, type_name,
    &DDS_TOPIC_QOS_DEFAULT, NULL, DDS_STATUS_MASK_NONE);
  assert(NULL != my_topic_in);

  my_topic_out = DDS_DomainParticipant_create_topic(
    my_participant, topic_name_out, type_name,
    &DDS_TOPIC_QOS_DEFAULT, NULL, DDS_STATUS_MASK_NONE);
  assert(NULL != my_topic_out);

  rc = DDS_Publisher_get_default_datawriter_qos_w_topic_name(
    my_publisher, &dw_qos, topic_name_out);
  assert(DDS_RETCODE_OK == rc);

  rc = DDS_PropertyQosPolicyHelper_assert_property(
    &dw_qos.property,
    "dds.data_writer.history.memory_manager.fast_pool.pool_buffer_max_size",
    "1024",
    DDS_BOOLEAN_FALSE);
  assert(DDS_RETCODE_OK == rc);

  rc = DDS_Subscriber_get_default_datareader_qos_w_topic_name(
    my_subscriber, &dr_qos, topic_name_in);
  assert(DDS_RETCODE_OK == rc);

  rc = DDS_PropertyQosPolicyHelper_assert_property(
    &dr_qos.property,
    "dds.data_reader.history.memory_manager.fast_pool.pool_buffer_max_size",
    "1024",
    DDS_BOOLEAN_FALSE);
  assert(DDS_RETCODE_OK == rc);

  my_reader = DDS_DomainParticipant_create_datareader(my_participant,
    DDS_Topic_as_topicdescription(my_topic_in),
    &dr_qos, NULL, DDS_STATUS_MASK_NONE);
  assert(NULL != my_reader);

  my_writer = DDS_DomainParticipant_create_datawriter(my_participant,
    my_topic_out, &dw_qos, NULL, DDS_STATUS_MASK_NONE);
  assert(NULL != my_writer);

  waitset = DDS_WaitSet_new();
  assert(NULL != waitset);

  my_reader_cond = DDS_Entity_get_statuscondition(
    DDS_DataReader_as_entity(my_reader));
  rc = DDS_StatusCondition_set_enabled_statuses(my_reader_cond,
    DDS_SUBSCRIPTION_MATCHED_STATUS | DDS_DATA_AVAILABLE_STATUS);
  assert(DDS_RETCODE_OK == rc);
  
  my_writer_cond = DDS_Entity_get_statuscondition(
    DDS_DataReader_as_entity(my_writer));
  rc = DDS_StatusCondition_set_enabled_statuses(my_writer_cond,
    DDS_PUBLICATION_MATCHED_STATUS);
  assert(DDS_RETCODE_OK == rc);

  exit_cond = DDS_GuardCondition_new();
  assert(NULL != exit_cond);

  rc = DDS_WaitSet_attach_condition(waitset,
    DDS_StatusCondition_as_condition(my_reader_cond));
  assert(DDS_RETCODE_OK == rc);

  rc = DDS_WaitSet_attach_condition(waitset,
    DDS_StatusCondition_as_condition(my_writer_cond));
  assert(DDS_RETCODE_OK == rc);

  rc = DDS_WaitSet_attach_condition(waitset,
    DDS_GuardCondition_as_condition(exit_cond));
  assert(DDS_RETCODE_OK == rc);

  treader = StringDataReader_narrow(my_reader);
  assert(NULL != treader);

  twriter = StringDataWriter_narrow(my_writer);
  assert(NULL != twriter);


  g_props.graph_participant = my_participant;
  graph = RTIROS2_Graph_new(&g_props);
  assert(NULL != graph);

  node_handle = RTIROS2_Graph_register_local_node(
    graph, "hello_c_adapter", NULL, my_participant);
  assert(RTIROS2_GraphNodeHandle_INVALID != node_handle);

  endp_handle = RTIROS2_Graph_register_local_subscription(
    graph, node_handle, my_reader);
  assert(RTIROS2_GraphEndpointHandle_INVALID != endp_handle);

  endp_handle = RTIROS2_Graph_register_local_publisher(
    graph, node_handle, my_writer);
  assert(RTIROS2_GraphEndpointHandle_INVALID != endp_handle);

  while (active)
  {
    rc = DDS_WaitSet_wait(waitset, &active_conditions, &wait_timeout);
    assert(DDS_RETCODE_OK == rc);

    active_conditions_len = DDS_ConditionSeq_get_length(&active_conditions);
    for (i = 0; i < active_conditions_len; i++)
    {
      active_cond = *DDS_ConditionSeq_get_reference(&active_conditions, i);
      if (active_cond == DDS_StatusCondition_as_condition(my_reader_cond))
      {
        status_changes = DDS_Entity_get_status_changes(
          DDS_DataReader_as_entity(my_reader));
        if (status_changes & DDS_SUBSCRIPTION_MATCHED_STATUS)
        {
          rc = DDS_DataReader_get_subscription_matched_status(
            my_reader, &sub_match_status);
          assert(DDS_RETCODE_OK == rc);

          printf("DataReader match status changed:\n");
          printf("  total: %d (%d), current: %d (%d)\n",
            sub_match_status.total_count,
            sub_match_status.total_count_change,
            sub_match_status.current_count,
            sub_match_status.current_count_change);
        }
        else if (status_changes & DDS_DATA_AVAILABLE_STATUS)
        {
          do {
            rc = StringDataReader_take(
              treader,
              &data_seq,
              &info_seq,
              DDS_LENGTH_UNLIMITED,
              DDS_ANY_VIEW_STATE,
              DDS_ANY_SAMPLE_STATE,
              DDS_ANY_INSTANCE_STATE);
            assert(DDS_RETCODE_OK == rc || DDS_RETCODE_NO_DATA == rc);
            if (DDS_RETCODE_NO_DATA == rc)
            {
              continue;
            }

            data_len = StringSeq_get_length(&data_seq);
            for (j = 0; j < data_len; j++)
            {
              info = DDS_SampleInfoSeq_get_reference(&info_seq, j);
              if (!info->valid_data)
              {
                continue;
              }
              data = StringSeq_get_reference(&data_seq, j);
              printf("Forwarding data: '%s'\n", data->data);

              rc = StringDataWriter_write(twriter, data, &DDS_HANDLE_NIL);
              assert(DDS_RETCODE_OK == rc);
            }
            rc = StringDataReader_return_loan(
              treader, &data_seq, &info_seq);
            assert(DDS_RETCODE_OK == rc);
          } while (DDS_RETCODE_OK == rc);
        }
      }
      else if (active_cond == DDS_StatusCondition_as_condition(my_writer_cond))
      {
        status_changes = DDS_Entity_get_status_changes(
          DDS_DataWriter_as_entity(my_writer));
        if (status_changes & DDS_PUBLICATION_MATCHED_STATUS)
        {
          rc = DDS_DataWriter_get_publication_matched_status(
            my_writer, &pub_match_status);
          assert(DDS_RETCODE_OK == rc);

          printf("DataWriter match status changed:\n");
          printf("  total: %d (%d), current: %d (%d)\n",
            pub_match_status.total_count,
            pub_match_status.total_count_change,
            pub_match_status.current_count,
            pub_match_status.current_count_change);
        }
      }
      else if (active_cond == DDS_GuardCondition_as_condition(exit_cond))
      {
        active = DDS_BOOLEAN_FALSE;
      }
    }
  }

  DDS_DomainParticipant_delete_contained_entities(my_participant);
  DDS_DomainParticipantFactory_delete_participant(factory, my_participant);
  DDS_DomainParticipantFactory_finalize_instance();
  DDS_DataWriterQos_finalize(&dw_qos);
  DDS_DataReaderQos_finalize(&dr_qos);
  return 0;
}