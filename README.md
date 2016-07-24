# glTFBastard
A C++ [glTF](https://github.com/KhronosGroup/glTF) document parser quickly put together. It simply takes in a string representing the json document and returns a document structure.
* MIT licensed.
* Requires C++11 and STL.
* Uses [json-parser](https://github.com/udp/json-parser).
* I currently use it to load glTF scenes on Android and Windows.

![AndroidDemo](https://github.com/rcashie/glTFBastard/blob/master/images/Android.png) 
![WindowsDemo](https://github.com/rcashie/glTFBastard/blob/master/images/Windows.png)

## Example
````c++
std::string source = ReadEntireFile("/Duck/glTF/Duck.gltf")

std::string error;
std::shared_ptr<const glTFBastard::glTF> doc = glTFBastard::Parse(source, error);

if (!doc) {
  PrintError("glTF load error: %s", error.c_str());
  abort();
}
````

## Missing (TODO's)
* Animation parsing.
* Asset parsing.
* Technique states.
