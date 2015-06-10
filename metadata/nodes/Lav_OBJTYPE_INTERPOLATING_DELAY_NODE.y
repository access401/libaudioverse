properties:
  Lav_DELAY_DELAY:
    name: delay
    type: float
    default: 0.0
    range: dynamic
    doc_description: |
      The delay of the delay line in seconds.
      The range of this property depends on the maxDelay parameter to the constructor.
  Lav_DELAY_INTERPOLATION_TIME:
    name: interpolation_time
    type: float
    default: 1.0
    range: [0.001, INFINITY]
    doc_description: |
      The time it takes the delay line to get to the new position.
      
      Shorter times cause higher pitch bends.
  Lav_DELAY_DELAY_MAX:
    name: delay_max
    type: float
    read_only: true
    doc_description: |
      The max delay as set at the node's creation time.
inputs:
  - [constructor, "The signal to delay."]
outputs:
  - [constructor, "The delayed signal."]
doc_name: dopplering delay line
doc_description: |
  Implements a doplering delay line.
  Delay lines have uses in echo and reverb, as well as many more esoteric effects.
  This delay line is specifically useful when the pitch change it introduces is beneficial, namely with doppler.