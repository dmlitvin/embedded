idf_component_register(SRCS "main.cpp"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES esp_driver_gpio esp_timer log)

# ESP-IDF appends its C++26 default after CMake's standard flag.
target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu++17)
