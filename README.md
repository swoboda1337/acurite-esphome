ESPHome SX127x driver + AcuRite OOK signal decoder. Allows for an easy integration of AcuRite sensors into Home Assistant. Tested with the LILYGO LoRa32 V2.1_1.6 board.

The AcuRite and the SX127x components are separate. One provides data on a GPIO and the other uses that data. The AcuRite component can be used with other radios and the SX127x component can be used with other sensors. 

The AcuRite component can be run without any sensors configured. Data received from all sensors will still be logged, this can be done to determine ids before setting up the sensor yaml.

Binary rain sensor is true if there was any rain in the last 15 minutes (at least one rain sensor needs to be configured for this to work).

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
    
    acurite:
      pin: GPIO32

    binary_sensor:
      - platform: acurite
        rain:
          name: "Backyard Rainfall"

    sensor:
      - platform: acurite
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

