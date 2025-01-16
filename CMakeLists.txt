cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(rgb_led_controller)

set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/examples/common_components/esp_zigbee)