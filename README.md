ESPHome SX127X driver + AcuRite OOK signal decoder. Allows for an easy intergration of AcuRite sensors into Home Assistant. Tested with the LILYGO LoRa32 V2.1_1.6 board.

The aucrite and the sx127x components are seporate. One provides data on a GPIO and the other uses that data. The acurite can be used with other radios and the sx127x component can be used with other sensors. 

If you run the acurite component without sensors it will print ids of all devices it finds. 

Example yaml to use in esphome device config:
    
    external_components:
      - source:
          type: git
          url: https://github.com/swoboda1337/acurite-esphome
          ref: main
        refresh: 0d
    
    time:
      - platform: homeassistant
        on_time:
          - seconds: 0
            minutes: 0
            hours: 0
            then:
              lambda: |-
                id(acurite_sensor).zero_rain_sensors();
    
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
        id: acurite_sensor
        pin: GPIO32
        devices:
          - device_id: 0x1d2e
            temperature:
              name: "AcuRite Temperature 1"
            humidity:
              name: "AcuRite Humidity 1"
          - device_id: 0x1fd2
            temperature:
              name: "AcuRite Temperature 2"
            humidity:
              name: "AcuRite Humidity 2"
          - device_id: 0x2838
            rain:
              name: "AcuRite Rain"

