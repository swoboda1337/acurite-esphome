ESPHome AcuRite OOK signal decoder, allowing for an easy integration of 433 mhz AcuRite weather sensors into Home Assistant.

The AcuRite component can be run without any sensors configured. Data received from all sensors will still be logged, this can be done to determine ids before setting up the sensor yaml.

Binary rain sensor is true if there was any rain in the last 15 minutes (at least one rain sensor needs to be configured for this to work).

A 433 mhz board that provides OOK data over a GPIO is required for this component to work. This component has been tested with the LILYGO LoRa32 V2.1_1.6 board using my [sx127x](https://github.com/swoboda1337/sx127x-esphome) component.

Example yaml to use in esphome device config:
    
    external_components:
      - source:
          type: git
          url: https://github.com/swoboda1337/acurite-esphome
          ref: main
        refresh: 1d
    
    time:
      - platform: homeassistant
    
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

