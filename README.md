ESPHome SX127X driver + AcuRite OOK signal decoder. Allows for an easy intergration of AcuRite sensors into Home Assistant. Tested with the LILYGO LoRa32 V2.1_1.6 board.

Example yaml to use in esphome device config:

    external_components:
      - source:
          type: git
          url: https://github.com/swoboda1337/acurite-esphome
          ref: main
    
    spi:
      clk_pin: GPIO5
      mosi_pin: GPIO27
      miso_pin: GPIO19
      interface: hardware
    
    sensor:
      - platform: acurite
        temperature:
          name: "AcuRite Temperature"
        humidity:
          name: "AcuRite Humidity"
        nss_pin: GPIO18
        rst_pin: GPIO23
        dio2_pin: GPIO32
