# GUI Application with OpenGL and Dear ImGui

Authors: Jonathan Elgarisi & Hagi Debby

## GUI Application with OpenGL and Dear ImGui

This project demonstrates the creation of a graphical application using GLFW, OpenGL, and Dear ImGui. The application provides detailed information about League of Legends champions, items, and summoner spells, including images and text details fetched from JSON files.

## Demo Video 

https://github.com/user-attachments/assets/68dda149-fe66-4bb4-9360-fa2915ec4c2f

## Features
- Initialize and manage a GLFW window.
- Render GUI elements using Dear ImGui.
- Load and display textures.
- Fetch and display data from the League of Legends API using nlohmann::json and httplib.
- Implement custom styles for GUI elements.
- Display detailed information about champions, items, and summoner spells, including images and textual descriptions.

## Third-Party Libraries Used
- [GLFW](https://www.glfw.org/): For creating and managing windows, OpenGL contexts, and input.
- [OpenGL](https://www.opengl.org/): For rendering graphics.
- [Dear ImGui](https://github.com/ocornut/imgui): For creating graphical user interfaces.
- [stb_image](https://github.com/nothings/stb/blob/master/stb_image.h): For loading images.
- [curl](https://curl.se/libcurl/): For fetching data from the web.
- [nlohmann::json](https://github.com/nlohmann/json): For parsing JSON data.
- [httplib](https://github.com/yhirose/cpp-httplib): For making HTTP requests.

## How to Run
Download the installer (LoLinfoAppSetup.msi) and simply run it, then you can use the shortcut in your desktop.
Virus total scan - [here](https://www.virustotal.com/gui/file/39330c96deef5aeec3e71441611e442e37accd7fd2e2b8fa0cc8057532cb0693?nocache=1) note that there are few false - positives.


## Functions
- Info about Champions, Items and Summoner's Spells.

## Output

The program outputs the GUI elements and data fetched from the League of Legends API. It provides visual feedback and interactions through the GUI window.

## Project Files
- `main.cpp`: Contains the main function to initialize and run the GUI application.
- `GuiManager.cpp`: Implementation of the GUI manager for handling the GUI window and rendering.
- `GuiManager.h`: Header file for the GUI manager with class definitions and function signatures.
- `DataManager.cpp`: Implementation of the data manager for fetching and processing data from the API.
- `DataManager.h`: Header file for the data manager with class definitions and function signatures.
- `README.txt`: The text file you are currently reading.

## License

MIT - https://choosealicense.com/licenses/mit/

By following this README, you can compile and run the GUI application, which demonstrates how to create a graphical application using GLFW, OpenGL, and Dear ImGui, fetch and display data from the League of Legends API, and implement custom GUI styles.
