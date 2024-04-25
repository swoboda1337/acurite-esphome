ESPHome AcuRite OOK signal decoder, allowing for an easy integration of 433 MHz AcuRite weather sensors into Home Assistant.

The AcuRite component can be run without any sensors configured. Data received from all sensors will still be logged, this can be done to determine ids before setting up the sensor yaml.

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
              force_update: true
              filters:
                - timeout: 5min
                - or:
                  - throttle: 30min
                  - delta: 0.01
            humidity:
              name: "AcuRite Humidity 1"
              force_update: true
              filters:
                - timeout: 5min
                - or:
                  - throttle: 30min
                  - delta: 0.01
          - device: 0x1fd2
            temperature:
              name: "AcuRite Temperature 2"
              force_update: true
              filters:
                - timeout: 5min
                - or:
                  - throttle: 30min
                  - delta: 0.01
            humidity:
              name: "AcuRite Humidity 2"
              force_update: true
              filters:
                - timeout: 5min
                - or:
                  - throttle: 30min
                  - delta: 0.01
          - device: 0x2838
            rain:
              name: "AcuRite Rainfall"
              force_update: true
              filters:
                - timeout: 5min
                - throttle: 50sec

It is recommended to use force_update so any statistics sensors in Home Assistant will work correctly. For example if the humidity stays at 99% all day, without force_update, a max/min statistics sensor would show unavailable as no data points will be logged. Using the delta and throttle filters will limit the amount of data being sent up and stored. 

Rain sensor values will only go up and never decrease (sensor state is stored in nvm). Note if you want to add a multiply filter for calibration it's easier to do before the sensor is added. If you apply calibration later on its best to add an offset filter to adjust the value back to where it should be. It's recommended to comment out the rain sensor in the yaml before calibration, reset the rain sensor after, update the filters and then re-enable the rain sensor. That way the resulting total will be unchanged and won't affect any statistics in Home Assistant.

An increasing total for rainfall is not very useful by itself. Here are some example sensors that can be added to Home Assistant's configuration.yaml

These sensors are window based and will calculate how much rain had fallen within the window:

    sensor:
      - platform: statistics
        name: "AcuRite Rainfall 15Min"
        entity_id: sensor.acurite_rainfall
        state_characteristic: change
        max_age:
          minutes: 15
      - platform: statistics
        name: "AcuRite Rainfall 1Hr"
        entity_id: sensor.acurite_rainfall
        state_characteristic: change
        max_age:
          hours: 1
      - platform: statistics
        name: "AcuRite Rainfall 24Hr"
        entity_id: sensor.acurite_rainfall
        state_characteristic: change
        max_age:
          hours: 24

These sensors will reset at specific times which is useful for daily, weekly or monthly:

    utility_meter:
      test_daily:
        source: sensor.acurite_rainfall
        name: AcuRite Rainfall Daily
        cycle: daily
      test_weekly:
        source: sensor.acurite_rainfall
        name: AcuRite Rainfall Weekly
        cycle: weekly
      test_monthly:
        source: sensor.acurite_rainfall
        name: AcuRite Rainfall Monthly
        cycle: monthly

A binary moisture sensor is also possible (can also be added graphically):

    template:
      - trigger:
          - platform: state
            entity_id: sensor.acurite_rainfall_15min 
        binary_sensor:
          - name: "AcuRite Rainfall Moisture"
            device_class: "moisture"
            state: "{{states('sensor.acurite_rainfall_15min')|float > 0}}"
