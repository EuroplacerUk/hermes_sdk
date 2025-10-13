NOTE: This is a clone of the standard open source SDK, the visual studio project files have been updated to use vcpkg to obtain dependencies and allow for an easier build.

NC - ignore the instructions below. Full integration of the SDK with the iiPS solution has been achieved.
Converted the project from a DLL project to a Universal Windows DLL project.
Reset output directories to ease linking.
Used NuGet for pugixml and boost.


Follow the instructions from https://vcpkg.io to install vcpkg and integrate into visual studio.

Dependencies include pugixml, boost-asio and boost-varient for windows x64.

- vcpkg install pugixml --triplet x64-windows
- vcpkg install boost-asio --triplet x64-windows
- vcpkg install boost-variant --triplet x64-windows
- vcpkg install boost-test --triplet x64-windows

# Hermes

"The Hermes Standard for vendor-independent machine-to-machine communication in SMT assembly" is a new, non-proprietary open protocol, based on TCP/IP- and XML, that takes exchange of PCB related data between the different machines in electronics assembly lines to the next level. It was initiated and developed by a group of leading equipment suppliers, bundling their expertise in order to achieve a great step towards advanced process integration. And the story continues: The Hermes Standard Initiative is open to all equipment vendors who want to actively participate in bringing the benefits of Industry 4.0 to their customers.

# Contents

The GIT-repository contains following directories:

- References - storage place for external data and souces the Hermes library depends on (required only with Windows)
- src - full sources of the Hermes library (Windows and Linux)
- test - a test application for the Hermes library (currently only for Windows)


