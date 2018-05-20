# BUILD STATUS
[![Build status](https://ci.appveyor.com/api/projects/status/mllnn7jx72ge8u6s/branch/master?svg=true)](https://ci.appveyor.com/project/BackTrak/allegiance)

# Installation

Download and install the lastest "Visual Studio Community Edition" : https://www.visualstudio.com/downloads/

Download and install the latest Microsoft DirectX SDK (jun 2010): https://www.microsoft.com/en-us/download/details.aspx?id=6812 (if you don't want to install it, just extract the DirectX `include` and `lib` folders somewhere and set the environment variable `DXSDK_DIR` to their parent directory)

Launch Visual Studio and open the main solution `src\VS2017\Allegiance.sln`

# Main projects

* Allegiance (`WinTrek` folder) : the main client program
* Server (`FedSrv` folder) : the game server
* Lobby (`lobby` folder) : the game lobby
* IGC (`Igc` floder): game logic library

# Sub projects
* `zlib`: various low level libraries used by all projects
* `engine`: 3d gfx engine
* `effect`: fx engine above `engine`
* `clintlib`: client library
* `utility` (nb: folder name is _Utility) : various high level libraries used by the main projects (notably: network code and collision detection)
* `soundengine`: sound engine
* `training`: single player training missions 

# Obsolete (or about to be) projects
* AGC
* AllSrvUI
* AutoUpdate
* cvh
* mdlc
* reloader

# Links
* Forums: https://www.freeallegiance.org/forums/

* Discord : https://discord.gg/TXGmynB

* Kage Code Documentation: http://www.freeallegiance.org/FAW/index.php/Code_documentation

* Dropbox build instructions: https://paper.dropbox.com/doc/FreeAllegiance-build-instructions-LVrOqhj4PnD0UWkohmxmW

* Trello Bug Reports/Feature Request: https://trello.com/b/0ApAZt6p/steam-release-testing-dx9
