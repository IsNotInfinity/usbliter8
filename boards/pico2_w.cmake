# boards/pico2_w.cmake
# Configuración específica para Raspberry Pi Pico 2W

add_compile_definitions(
    BOARD_NAME="Raspberry Pi Pico 2W"
    PIO_USB_DP_PIN_DEFAULT=12
    LED_SINGLE_COLOR=1
    LED_PIN=PICO_DEFAULT_LED_PIN
)