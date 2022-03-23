# ROS 2 helper for Connext DDS applications

`rticonnextdds-ros2-adapter` is an open-source C/C++ library to help
applications built with Connext DDS interact with the
[ROS 2 graph](https://docs.ros.org/en/rolling/Tutorials/Understanding-ROS2-Nodes.html#the-ros-2-graph) and other components of the ROS 2 ecosystem.

## Building

`rticonnextdds-ros2-adapter` provides native APIs for both C and C++.

The libraries can be built using CMake:

```sh
# Clone this repository
git clone https://github.com/asorbini/rticonnextdds-ros2-adapter

# Create a build directory
mkdir build

# Load Connext DDS for your architecture into the environment,
# e.g. load Connext DDS for Linux 64 bit using the provided env script.
source ~/rti_connext_dds-6.0.1/resource/scripts/rtisetenv_x64Linux4gcc7.3.0.bash

# Configure project using cmake: specify a custom install prefix, and
# optionally build included tests and examples.
cmake rticonnextdds-ros2-adapter \
  -Bbuild \
  -DCMAKE_INSTALL_PREFIX=install \
  -DBUILD_TESTING=ON \
  -DBUILD_EXAMPLES=ON \

# Invoke native build tool using cmake and install artifacts.
cmake --build build -- install
```

## Examples

The included examples will only be built if the repository is configured with
`-DBUILD_EXAMPLES=ON`.

You can find all examples in the installation directory under `bin/`.
In order to run them, you will have to add `install/lib` to your shared library
path, e.g. on Linux (assuming that you used `-DCMAKE_INSTALL_PREFIX=install` when configuring the project):

```sh
export LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:install/lib"
```

The examples are written to work out of the box with some ROS 2 applications. In order to run them, you will need a recent version of [ROS 2](https://docs.ros.org/en/rolling/Installation.html) with [rmw_connextdds](https://github.com/ros2/rmw_connextdds) installed.

### hello_c_adapter, hello_cpp_adapter

- Examples [hello_c_adapter](examples/c/hello_c_adapter/hello_c_adapter.c) and [hello_c_adapter](examples/cpp/hello_cpp_adapter/hello_cpp_adapter.cpp) will listen for messages on ROS 2 topic `"chatter"` and
  duplicate them on topic "chatter_dup".

- Use the `talker` node from `demo_nodes_cpp` to generate data,
  then start the examples to read it.

  ```sh
  # Start talker in background (or use a different terminal).
  RMW_IMPLEMENTATION=rmw_connextdds ros2 run demo_nodes_cpp talker &

  install/bin/hello_cpp_adapter
  ```

- You can use `rqt`'s "Node Graph" plugin to visualize the example node and
  its endpoints:

  ![hello_cpp_adapter in rqt](docs/static/hello_cpp_adapter_rqt.png)

### parameters_client_cpp

- Example [parameters_client_cpp](examples/cpp/parameters_cpp/parameters_client_cpp.cpp) will send a request to the `"list_parameters"` service
  of ROS 2 node `"talker"`.

- Start `demo_nodes_cpp/talker`, then start the client to query it:

  ```sh
  # Start talker in background (or use a different terminal).
  RMW_IMPLEMENTATION=rmw_connextdds ros2 run demo_nodes_cpp talker &

  install/bin/parameters_client_cpp
  ```

### parameters_service_cpp

- Example [parameters_service_cpp](examples/cpp/parameters_cpp/parameters_service_cpp.cpp) will advertise a (fake) `"list_parameters"` service.

- Start the service, then use the `ros2 service` utility to interact with it

  ```sh
  # Start service in background (or use a different terminal).
  install/bin/parameters_service_cpp &

  # Query list of available services:
  RMW_IMPLEMENTATION=rmw_connextdds ros2 service list

  # Invoke the service:
  RMW_IMPLEMENTATION=rmw_connextdds ros2 service call \
    /foo/list_parameters rcl_interfaces/srv/ListParameters
  ```

- You can also use `rqt`'s "Service Call" plugin to interact with the service:

  ![parameters_service_cpp in rqt](docs/static/parameters_service_cpp_in_rqt.png)

## Unit tests

In order to build the included unit tests, the respository must have been
configured with `-DBUILD_TESTING=ON`.

Once built, you can ran all included tests using `ctest`:

```sh
(cd build/test && ctest)
```

## API Usage

The adapter library allows application to instantiate a local ROS 2 graph object, which they can then use to register local
DDS endpoints and have the graph announce them to other ROS 2 applications over the network.

In order to register their DDS endpoints, users must first associate their DomainParticipant with a ROS 2 Node identity, by
specifying a name and namespace for it.

After creating a local node, users can either register each endpoint by hand, or they can let the library inspect the node's
DomainParticipant to automatically register any endpoint whose
topic name follows the ROS 2 naming conventions.

The local graph object requires at least a DomainParticipant, which it
will use to create the DDS entities required to propagate the
ROS 2 graph information.

Only endpoints whose topic name follows the ROS 2 naming conventions
may be registered on the graph. The library offers some helper
functions to facilitate generation of conforming names.

### C API

The `rticonnextdds-ros2-adapter` C API is meant to be integrated into applications using Connext's C API.

Applications are expected to create an object of type `RTIROS2_Graph' and pass it a DomainParticipant to use through the properties object:

```c
#include "ndds/ndds_c.h"
#include "rticonnextdds_ros2_adapter/rticonnextdds_ros2_adapter_c.h"

/* The application will typically reuse its DomainParticipant,
   for example created manually from the participant factory */
DDS_DomainParticipant * my_participant = 
  DDS_DomainParticipantFactory_create_participant(DDS_TheParticipantFactory,
    0, &DDS_PARTICIPANT_QOS_DEFAULT, NULL, DDS_STATUS_MASK_NONE);


RTIROS2_GraphProperties props = RTIROS2_GraphProperties_INITIALIZER;
props.graph_participant = my_participant;
RTIROS2_Graph * graph = RTIROS2_Graph_new(&props);
```

Endpoints must be associated with a ROS 2 Node, so before registering them, users
must declare one or more nodes and save their handle for future reference.

Each node must be assigned a name, a namespace, and a DomainParticipant.

The namespace argument might be NULL, in which case the default namespace ("/")
will be used.

The participant argument can also be `NULL`, in which case the node will be
associated with the DomainParticipant used by the graph.

```c
const RTIROS2_GraphNodeHandle node_h =
  RTIROS2_Graph_register_local_node(graph, "my_node", NULL, NULL);
if (RTIROS2_GraphNodeHandle_INVALID == node_h)
{
  printf("failed to register node\n);
}
```

After registering a node, the application can use the API to map its DDS endpoints
to ROS 2 entities. For example, assuming the application created a DataReader
for topic `"rt/chatter"` (`my_reader`), the reader could be mapped to a
ROS 2 subscription for topic `"chatter"`:

```c
const RTIROS2_GraphEndpointHandle endp_h =
  RTIROS2_Graph_register_local_subscription(graph, node_h, my_reader);
if (RTIROS2_GraphEndpointHandle_INVALID == endp_h)
{
  printf("failed to register subscription\n);
}
```

Alternatively, the library can inspect the DomainParticipant associated with
a node and automatically register all endpoints that are found to be following
the ROS 2 naming conventions:

```c
DDS_ReturnCode_t rc = RTIROS2_Graph_inspect_node(graph, node_h);
if (DDS_RETCODE_OK != rc)
{
  printf("failed to inspect node\n);
}
```

Note that this automatic inspection will not work if you plan on declaring
multiple local nodes and having them share the same DomainParticipant, because
in this case the endpoints will be all associated with the first node to be
inspected. If you want to share the DomainParticipant between multiple nodes
you should manually register your endpoints for a finer grained control.

### C++ API

The `rticonnextdds-ros2-adapter` C++ API is meant to be integrated into applications
that are already using Connext's "modern" C++ API.

The C++ adapter API is built on top of the C implementation, and it exposes
very similar interfaces in a more C++ friendly variant.

Applications are expected to instantiate class `rti::ros2::Graph` with a
DomainParticipant:

```cpp
#include <dds/dds.hpp>
#include "rticonnextdds_ros2_adapter/rticonnextdds_ros2_adapter_cpp.hpp"

dds::domain::DomainParticipant my_participant(0);

rti::ros2::GraphProperties props;
props.graph_participant = my_participant;

rti::ros2::Graph graph(props);
```

Similarly to the C API, users are expected to map local DomainParticipants to
ROS 2 Nodes, and then use the node handles to map their DDS endpoints to ROS 2
entities.

```cpp
using namespace rti::ros2;

GraphNodeHandle node_h = graph.register_local_node("my_node");
GraphEndpointHandle endp_h = graph.register_local_subscription(node_h, my_reader);
```

The C++ API can also automatically inspect a node's participant:

```cpp
try {
  graph.inspect_local_node(node_h);
} catch (dds::core::Exception & e)
{
  std::cout << "DDS exception: " << e.what();
}
```

The main difference with the C API is that in the C++ API, function which returned
`DDS_ReturnCode_t` in C will instead throw an exception
derived from `dds::core::Exception` in case of error.

