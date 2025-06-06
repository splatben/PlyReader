## Contents
* An Godot project for test (`demo/`)
* godot-cpp as a submodule (`godot-cpp/`)
* preconfigured source files for C++ development of the GDExtension (`src/`)

## Usage

For getting started after cloning your own copy to your local machine, you should: 
* initialize the godot-cpp git submodule via `git submodule update --init`
* register the classes you want Godot to interact with inside the `register_types.cpp` file in the initialization method (here `initialize_gdextension_types`) in the syntax `GDREGISTER_CLASS(CLASS-NAME);`.(if you add new classes)
* compile with `scons platform=window target=template_release `
