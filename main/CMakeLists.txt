idf_component_register(SRCS "esp32_h2_zigbee_led.c"
                    INCLUDE_DIRS "."
                    REQUIRES nvs_flash driver)

target_include_directories(${COMPONENT_LIB} PRIVATE
   "../managed_components/espressif__esp-zigbee-lib/include"
   "../managed_components/espressif__esp-zboss-lib/include"
)