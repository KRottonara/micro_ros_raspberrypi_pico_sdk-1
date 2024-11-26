#include <stdio.h>

#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <std_msgs/msg/int32.h>
#include <rmw_microros/rmw_microros.h>

#include "pico/stdlib.h"
#include "pico_uart_transports.h"

#include <geometry_msgs/msg/twist.h>    // Include the message type


// Define the LED pin
const uint LED_PIN = 25;

//Define GP20 pin for switch
const uint SWITCH_PIN = 20;

// Declare the publisher and message
rcl_publisher_t publisher;
geometry_msgs__msg__Twist msg;

// Timer callback function to publish the message
void timer_callback(rcl_timer_t *timer, int64_t last_call_time)
{
    // rcl_ret_t ret = rcl_publish(&publisher, &msg, NULL);
    msg.linear.x += 0.1;
    bool switch_state = gpio_get(SWITCH_PIN); 
    printf("Switch state: %d\r\n", switch_state);
}

int main()
{
    // Set custom transport for micro-ROS
    rmw_uros_set_custom_transport(
        true,
        NULL,
        pico_serial_transport_open,
        pico_serial_transport_close,
        pico_serial_transport_write,
        pico_serial_transport_read
    );

    // Initialize the LED pin
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    
    // Initialize the switch pin
    gpio_init(SWITCH_PIN);
    gpio_set_dir(SWITCH_PIN, GPIO_IN);
    gpio_pull_up(SWITCH_PIN);

    // Declare necessary variables for micro-ROS
    rcl_timer_t timer;
    rcl_node_t node;
    rcl_allocator_t allocator;
    rclc_support_t support;
    rclc_executor_t executor;

    // Get the default allocator
    allocator = rcl_get_default_allocator();

    // Wait for agent successful ping for 2 minutes
    const int timeout_ms = 1000; 
    const uint8_t attempts = 120;

    rcl_ret_t ret = rmw_uros_ping_agent(timeout_ms, attempts);

    if (ret != RCL_RET_OK)
    {
        // Unreachable agent, exiting program
        return ret;
    }

    // Initialize support structure
    rclc_support_init(&support, 0, NULL, &allocator);

    // Initialize the node
    rclc_node_init_default(&node, "pico_node", "", &support);

    // Initialize the publisher
    rclc_publisher_init_default(
        &publisher,
        &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(geometry_msgs, msg, Twist),
        "pico_publisher");

    // Initialize the timer
    rclc_timer_init_default(
        &timer,
        &support,
        RCL_MS_TO_NS(50),
        timer_callback);

    // Initialize the executor
    rclc_executor_init(&executor, &support.context, 1, &allocator);
    rclc_executor_add_timer(&executor, &timer);

    // Turn on the LED
    gpio_put(LED_PIN, 1);

    // Initialize the message data
    msg.linear.x = 0;

    // Variable to store the previous switch state
    // bool previous_switch_state = gpio_get(SWITCH_PIN);

    // Spin the executor to handle callbacks
    while (true)
    {
        // Read the switch state
        // bool switch_state = gpio_get(SWITCH_PIN);

        // // Print the switch state only if it changes
        // if (switch_state != previous_switch_state)
        // {
        //     printf("Switch state: %d\r\n", switch_state);
        //     previous_switch_state = switch_state;
        // }

        rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100));

        // // Add a small delay to limit the frequency of `printf` calls
        // sleep_ms(1000);
    }

    return 0;
}
