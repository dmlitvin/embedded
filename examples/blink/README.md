# Blink на ESP-IDF

Реалізував клас `Led` зі станами `On` / `Off` і перемиканням кожні 500 мс.
Параметри виніс у `static constexpr`, час перевіряю в superloop через
`esp_timer_get_time()`. Без Arduino, глобальних змінних і динамічної пам’яті.

LED підключений до GPIO4 через резистор 330 Ω, катод — до GND.

```sh
get_idf
cd examples/blink
idf.py build
idf.py -p /dev/cu.usbmodem1101 flash monitor
```
