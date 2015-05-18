callbacks: [listening]
inputs:
  - [constructor, "The audio which will be bassed to the associated callback."]
outputs:
  - [constructor, "The same audio as connected to the input without modification."]
doc_name: graph listener
doc_description: |
  This node defines a callback which is called with the audio data passing through this node for the current block.
  Graph listeners can be used to implement cases wherein the app wishes to capture audio from some part of the graph of connected nodes in realtime.