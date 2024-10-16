// Copyright 2016 Open Source Robotics Foundation, Inc.
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

#include <chrono>
#include <memory>
#include <chrono>
#include <cinttypes>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "example_interfaces/srv/add_two_ints.hpp"

#include <rclcpp_lifecycle/lifecycle_node.hpp>
#include <lifecycle_msgs/msg/state.hpp>
#include <lifecycle_msgs/msg/transition.hpp>

#include "rclcpp/rclcpp.hpp"

using AddTwoInts = example_interfaces::srv::AddTwoInts;

using namespace std::chrono_literals;

/* This example creates a subclass of Node and uses std::bind() to register a
 * member function as a callback from the timer. */

class MinimalTimer : public rclcpp_lifecycle::LifecycleNode
{
public:
using CallbackReturn =
    rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn;
  MinimalTimer()
  : rclcpp_lifecycle::LifecycleNode("minimal_timer")
  {
    client_ = create_client<AddTwoInts>("add_two_ints");
    // Timer that is always running and sends a client request
    timer_ = create_wall_timer(
      500ms, std::bind(&MinimalTimer::timer_callback, this));
  }

CallbackReturn on_activate(const rclcpp_lifecycle::State& previous_state)
{
  RCLCPP_INFO(this->get_logger(), "Activated");
  // Timer created on a lifecycle callback that will _stop executing_ when timer_ is deleted!
  timer2_ = create_wall_timer(
    100ms, std::bind(&MinimalTimer::timer2_callback, this));
  return CallbackReturn::SUCCESS;
}

private:
  void timer_callback()
  {
    queue_async_request(1,2);
  }

  void timer2_callback()
  {
    RCLCPP_INFO(this->get_logger(), "Hello, world!");
  }

  void
  queue_async_request(int64_t a, int64_t b)
  {
    RCLCPP_INFO(this->get_logger(), "Sending request!");
    auto request = std::make_shared<AddTwoInts::Request>();
    request->a = a;
    request->b = b;

    // We give the async_send_request() method a callback that will get executed once the response
    // is received.
    // This way we can return immediately from this method and allow other work to be done by the
    // executor in `spin` while waiting for the response.
    using ServiceResponseFuture =
      rclcpp::Client<example_interfaces::srv::AddTwoInts>::SharedFutureWithRequest;
    auto response_received_callback =
      [logger = this->get_logger(), this](ServiceResponseFuture future) {
        auto request_response_pair = future.get();
        RCLCPP_INFO(
          logger,
          "Result of %" PRId64 " + %" PRId64 " is: %" PRId64,
          request_response_pair.first->a,
          request_response_pair.first->b,
          request_response_pair.second->sum);
          if (timer_)
          {
            timer_.reset();
          }
      };
    auto result = client_->async_send_request(
      request, std::move(response_received_callback));
  }

  rclcpp::Client<AddTwoInts>::SharedPtr client_;
  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::TimerBase::SharedPtr timer2_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
    rclcpp::executors::SingleThreadedExecutor exe;

  std::shared_ptr<MinimalTimer> lc_node =
    std::make_shared<MinimalTimer>();

  exe.add_node(lc_node->get_node_base_interface());

  exe.spin();

  rclcpp::shutdown();
  return 0;
}
