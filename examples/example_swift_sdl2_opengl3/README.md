# Swift (5.9+) SDL/OpenGL Example

An example using Swift to import Dear IMGUI directly using the new C++ interoperability features introduced in Swift 5.9.

Some small changes to the C++ SDL/GL example made to account for a lack of C variadics and direct pointer access to C++ types like `ImVec4`.

Uses SDL and OpenGL for Linux compatibility, but could be used with Metal and native iOS/macOS window system integration if required.

The IMGUI sources are included as relative symlinks for ease of building, these will need to be updated/replaced with the actual files including backends to include in your own projects.






