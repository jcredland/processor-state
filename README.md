# processor-state

A variation on AudioProcessorValueTreeState.  It's for JUCE plugins.

It:
* allows parameter information to be saved and loaded.
* provides a generic mechanism for saving and loading non-scalar data, e.g.  file names, envelope data, zone maps
* should be less prone to threading problems than AudioProcessorValueTreeState

Includes an example of how to load and save a preset which includes a parameter
and a filename, including how to load the file in a thread-safe manner when a
new preset is selected.
