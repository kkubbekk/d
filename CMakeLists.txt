   cmake_minimum_required(VERSION 3.16)
   include($ENV{IDF_PATH}/tools/cmake/project.cmake)
   project(ble_test
   REQUIRES esp_driver_gpio
   REQUIRES esp_event)