# jetson-embeddedUI
Example application running on Jetson Orin Nano. The daemon controls hardware devices attached to Jetson, and exposes them via a websocket. 

This is based on previous work by James Alley. 

 ### Dependencies

 The application has three dependencies outside of the standard C/C++ libraries.

 1. nlohmann/json
 1. libwebsockets

 ### User Interface

The application provides a web-based user interface for monitoring and controlling hardware peripherals.
To access the interface:

1. Open a web browser on a device connected to the same network as the Jetson.
2. Navigate to `http://<IP_ADDRESS>:7800`, where:
   - `<IP_ADDRESS>` is the IP address of the device running the application.
   If running on the same machine, use `localhost`.
   - The port number `7800` is specified in the `configuration/userSettings.json` file.


The interface updates in real-time (1Hz), reflecting the latest data from the Jetson.

Note: Ensure that your network settings allow access to the specified port (7800 by default) on the device running the application. If you're having trouble connecting, check your firewall settings and network configuration.

## Building

This application is built using cmake and gcc. You can install all tools and depedencies required by
running `apt-get install build-essential crossbuild-essential-arm64 cmake libgpio-dev libwebsockets-dev`.
Building the application has only been tested in Linux; Windows developers may need to modify these build
instructions. 

1) Run `mkdir build && cd build`
1) Configure the project using CMake: `cmake ..`
1) Build the project: `make`

### Build Options

The CMakeLists.txt file provides two options for building:


2. **ENABLE_LOGGING**: Enable testing mode
   - To enable: `cmake -DENABLE_LOGGING=ON ..`
   - This option adds the ENABLE_LOGS compile definition.

### Web Files

The application requires web files to be present in a specific location. After building, you need to copy the files from the `web` directory to `/var/www/webFiles`. You can do this with the following command: `sudo cp -r ../web/ /var/www/webFiles/`.

Make sure you have the necessary permissions to write to `/var/www/webFiles/`. You may need to create this directory if it doesn't exist: `sudo mkdir -p /var/www/webFiles`.

If running the application on your local machine the server IP address will likely be your loopback address. Modify
`/var/www/webFiles/index.html` to use the loopback address: `let socket = new WebSocket("ws://localhost:7800", "ws-protocol-text");`.
