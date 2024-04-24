ESPHome AcuRite OOK signal decoder, allowing for an easy integration of 433 MHz AcuRite weather sensors into Home Assistant.

The AcuRite component can be run without any sensors configured. Data received from all sensors will still be logged, this can be done to determine ids before setting up the sensor yaml.

Binary rain sensor is true if there was any rain in the last 15 minutes (at least one rain sensor needs to be configured for this to work).

A 433 MHz board that provides OOK data over a GPIO is required for this component to work. This component has been tested with the LILYGO LoRa32 V2.1_1.6 board using my [SX127x](https://github.com/swoboda1337/sx127x-esphome) component.

Example yaml to use in esphome device config:
    
    external_components:
      - source:
          type: git
          url: https://github.com/swoboda1337/acurite-esphome
          ref: main
        refresh: 1d
    
    acurite:

    remote_receiver:
      pin: GPIO32
      filter: 255us
      idle: 800us
      buffer_size: 100000b
      memory_blocks: 8

    sensor:
      - platform: acurite
        devices:
          - device: 0x1d2e
            temperature:
              name: "AcuRite Temperature 1"
              filters:
                - timeout: 5min
                - or:
                  - throttle: 30min
                  - delta: 0.01
            humidity:
              name: "AcuRite Humidity 1"
              filters:
                - timeout: 5min
                - or:
                  - throttle: 30min
                  - delta: 0.01
          - device: 0x1fd2
            temperature:
              name: "AcuRite Temperature 2"
              filters:
                - timeout: 5min
                - or:
                  - throttle: 30min
                  - delta: 0.01
            humidity:
              name: "AcuRite Humidity 2"
              filters:
                - timeout: 5min
                - or:
                  - throttle: 30min
                  - delta: 0.01
          - device: 0x2838
            rain:
              name: "AcuRite Rainfall"
              filters:
              force_update: true
              filters:
                - timeout: 5min
                - throttle: 50sec

