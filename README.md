# EDF-MDB-Viewer
An OpenGL based Model Viewer for the 'mdb' file format found in Sandlot's "Earth Defense Force" video games.

# Requirements
OpenGL libraries (GLM, GL, ect), SFML libraries.

# Operation
Running this software should open a menu, option 1 will print the operational directory, filtering for file formats the tool understands. You can also change directory, but cannot "go up" in the tree. Opening an MDB raw will defaut to loading the model with a texture defined as "texture.dds", however loading from an RAB will load the first model it can find within, and correctly apply textures.

Option 2 will load "./EDFData/IG_BASE502.mac" and associated .rab fie in scene viewer mode. This will also load "./EDFData/MISSION.RMPA".

Right now, the tool's "Model Viewer" mode only cares about the first "object" within an mdb, and cannot view secondary objects yet.

This tool is incredibly bare-bones and serves only as an example at this current time.

There may be a memory leak due to incomplete cleanup operations, use at your own risk.
