## Contents
* An Godot project for test (`demo/`)
* godot-cpp as a submodule (`godot-cpp/`)
* preconfigured source files for C++ development of the GDExtension (`src/`)

## Usage

For getting started after cloning your own copy to your local machine, you should: 
* initialize the godot-cpp git submodule via `git clone -b 4.4 https://github.com/godotengine/godot-cpp`
* register the classes you want Godot to interact with inside the `register_types.cpp` file in the initialization method (here `initialize_gdextension_types`) in the syntax `GDREGISTER_CLASS(CLASS-NAME);`.(if you add new classes)
* compile with `scons platform=windows target=template_release `
* only work on windows
* now you can wait for a long time
