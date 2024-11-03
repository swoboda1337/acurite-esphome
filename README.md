ESPHome AcuRite OOK signal decoder, allowing for an easy integration of 433 MHz AcuRite weather sensors into Home Assistant. At the moment the following devices are supported: temperature / humidity sensor, rain gauge, notos weather station, iris weather station, atlas weather station and lightning detector. Support for additional sensors can added if someone is available to test the changes. 

The AcuRite component can be run without any sensors configured. Data received from all sensors will still be logged, this can be done to determine ids before setting up the sensor yaml.

A 433 MHz board that provides OOK data over a GPIO is required for this component to work. This component has been tested with the LILYGO LoRa32 V2.1_1.6 board using my [SX127x](https://github.com/swoboda1337/sx127x-esphome) component. Note the small 433 MHz antennas that come with these boards work fine but are not ideal. The antenna gain is really poor and it’s too close to the Wi-Fi antenna which can cause glitches. A proper antenna like the Siretta Tango 9 has better range and doesn’t glitch when Wi-Fi transmits.

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
      filter: 100us
      idle: 800us
      buffer_size: 100000b
      memory_blocks: 8

    binary_sensor:
      - platform: acurite
        devices:
          - device: 0x1d2e
            battery_level:
              name: "Battery Level Temp 1"
          - device: 0x1fd2
            battery_level:
              name: "Battery Level Temp 2"
          - device: 0x2838
            battery_level:
              name: "Battery Level Rainfall"
          - device: 0x0083
            battery_level:
              name: "Battery Level Atlas"

    sensor:
      - platform: acurite
        devices:
          - device: 0x1d2e
            temperature:
              name: "Temperature 1"
              force_update: true
              filters:
                - timeout: 5min
                - or:
                  - throttle: 30min
                  - delta: 0.01
            humidity:
              name: "Humidity 1"
              force_update: true
              filters:
                - timeout: 5min
                - or:
                  - throttle: 30min
                  - delta: 0.01
          - device: 0x1fd2
            temperature:
              name: "Temperature 2"
              force_update: true
              filters:
                - timeout: 5min
                - or:
                  - throttle: 30min
                  - delta: 0.01
            humidity:
              name: "Humidity 2"
              force_update: true
              filters:
                - timeout: 5min
                - or:
                  - throttle: 30min
                  - delta: 0.01
          - device: 0x2838
            rain:
              name: "Rainfall"
              force_update: true
              filters:
                - timeout: 5min
                - throttle: 50sec
          - device: 0x0083
            speed:
              name: "Wind speed"
              filters:
                - timeout: 5min
                - or:
                  - throttle: 10min
                  - delta: 0.01
            direction:  
              name: "Wind direction"
              filters:
                - timeout: 5min
                - or:
                  - throttle: 10min
                  - delta: 0.01
            lightning:
              name: "Lightning Strikes"
              force_update: true
              filters:
                - timeout: 5min
                - or:
                  - throttle: 30min
                  - delta: 0.01
            distance:  
              name: "Lightning Distance"
              force_update: true
              filters:
                - timeout: 5min
                - or:
                  - throttle: 30min
                  - delta: 0.01
            uv:
              name: "UV index"
              filters:
                - timeout: 5min
                - or:
                  - throttle: 10min
                  - delta: 0.01
            lux:
              name: "Light Intensity"
              filters:
                - timeout: 5min
                - or:
                  - throttle: 10min
                  - delta: 0.01

It is recommended to use force_update so any statistics sensors in Home Assistant will work correctly. For example if the humidity stays at 99% all day, without force_update, a max/min statistics sensor would show unavailable as no data points will be logged. Using the delta and throttle filters will limit the amount of data being sent up and stored. 

As of November 2024, rain sensors no longer store anything in NVM and now output the current device total. This was done to make things more flexible. To restore the previous functionality of a lifetime total a lambda + two globals can be used:

    globals:
      - id: rainfall_total
        type: float
        restore_value: yes
      - id: rainfall_current
        type: float
        restore_value: yes

    sensor:
      - platform: acurite
        devices:
          - device: 0x2838
            rain:
              name: "Test Backyard Rainfall"
              force_update: true
              filters:
                - lambda: |-
                    // update rainfall state and save to nvm
                    if (x != id(rainfall_current)) {
                      float delta;
                      if (x >= id(rainfall_current)) {
                        delta = x - id(rainfall_current);
                      } else {
                        delta = x;
                      }
                      id(rainfall_total) += delta; // can apply a calibration value here
                      id(rainfall_current) = x;
                    }
                    return id(rainfall_total);
                - throttle: 50sec
                - timeout: 5min

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
      daily_rainfall:
        source: sensor.acurite_rainfall
        name: AcuRite Rainfall Daily
        always_available: true
        periodically_resetting: false
        cycle: daily
      weekly_rainfall:
        source: sensor.acurite_rainfall
        name: AcuRite Rainfall Weekly
        always_available: true
        periodically_resetting: false
        cycle: weekly
      monthly_rainfall:
        source: sensor.acurite_rainfall
        name: AcuRite Rainfall Monthly
        always_available: true
        periodically_resetting: false
        cycle: monthly

A binary moisture sensor can also be created using a template:

    {{states('sensor.acurite_rainfall_15min')|float > 0}}

For lightning detection the following example sensors that can be added to Home Assistant. These sensors will create a daily counter, daily closest distance, most recent distance and a binary sensor indicating if lightning has been detected recently. If there has been no strike today both closest and last will show 40km (max distance):

    utility_meter:
      daily_lightning:
        source: sensor.lightning_strikes
        name: "Lightning Strikes Daily"
        always_available: true
        periodically_resetting: true
        cron: "0 0 * * *"

    template:
      - trigger:
          - platform: time
            at: '00:00:00'
          - platform: state
            entity_id: sensor.lightning_distance
        sensor:
          - name: "Lightning Distance Last"
            device_class: distance
            unit_of_measurement: "km"
            state: >
                {% if trigger.platform == 'time' %}
                    {{ float(40) }}
                {% else %}
                    {% if states('sensor.lightning_strikes_daily') | float(0) | float > 0 %}
                        {{ states('sensor.lightning_distance') }}
                    {% else %}
                        {{ float(40) }}
                    {% endif %}
                {% endif %}
      - trigger:
          - platform: time
            at: '00:00:00'
          - platform: state
            entity_id: sensor.lightning_distance
        sensor:
          - name: "Lightning Distance Closest"
            device_class: distance
            unit_of_measurement: "km"
            state: >
                {% if trigger.platform == 'time' %}
                    {{ float(40) }}
                {% else %}
                    {% if states('sensor.lightning_strikes_daily') | float(0) | float > 0 %}
                        {{ [states('sensor.lightning_distance') | float(40), this.state | float(40)] | min }}
                    {% else %}
                        {{ float(40) }}
                    {% endif %}
                {% endif %}

    binary_sensor:
      - platform: trend
        sensors:
          lightning_active:
            friendly_name: Lightning Active
            entity_id: sensor.lightning_strikes
            device_class: power
            sample_duration: 1800
            min_gradient: 0.0001
            min_samples: 2
            max_samples: 30
