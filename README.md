ESPHome SX127X driver + AcuRite OOK signal decoder. Allows for an easy intergration of AcuRite sensors into Home Assistant. Tested with the LILYGO LoRa32 V2.1_1.6 board.

The aucrite and the sx127x components are separate. One provides data on a GPIO and the other uses that data. The acurite can be used with other radios and the sx127x component can be used with other sensors. 

If you run the acurite component without sensors it will print ids of all devices it finds. Rain sensor needs to be reset everyday or week, example yaml resets it at midnight.

Example yaml to use in esphome device config:
    
    external_components:
      - source:
          type: git
          url: https://github.com/swoboda1337/acurite-esphome
          ref: main
        refresh: 1d
    
    time:
      - platform: homeassistant
    
    spi:
      clk_pin: GPIO5
      mosi_pin: GPIO27
      miso_pin: GPIO19
    
    sx127x:
      nss_pin: GPIO18
      rst_pin: GPIO23
      frequency: 433920000
      bandwidth: 50_0kHz
      modulation: OOK
    
    sensor:
      - platform: acurite
        pin: GPIO32
        devices:
          - device: 0x1d2e
            temperature:
              name: "AcuRite Temperature 1"
            humidity:
              name: "AcuRite Humidity 1"
          - device: 0x1fd2
            temperature:
              name: "AcuRite Temperature 2"
            humidity:
              name: "AcuRite Humidity 2"
          - device: 0x2838
            rain_1hr:
              name: "AcuRite Rainfall 1Hr"
            rain_24hr:
              name: "AcuRite Rainfall 24Hr"
            rain_daily:
              name: "AcuRite Rainfall Daily"

