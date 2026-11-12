idf_component_register(SRCS "main.cpp" INCLUDE_DIRS "."
                       REQUIRES esp_driver_gpio esp_driver_uart esp_timer)
set_target_properties(${COMPONENT_LIB} PROPERTIES
                      CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS OFF)
target_compile_options(${COMPONENT_LIB} PRIVATE "-std=c++17")
