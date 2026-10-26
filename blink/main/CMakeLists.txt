idf_component_register(SRCS "blink.cpp" "led.cpp"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES esp_driver_gpio esp_timer freertos)
target_compile_features(${COMPONENT_LIB} PRIVATE cxx_std_17)
