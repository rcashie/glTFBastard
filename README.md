# glTFBastard
* MIT licensed.
* Requires C++11 and STL.
* Uses [json-parser](https://github.com/udp/json-parser).
* I currently use it to load glTF scenes on Android and Windows.

![AndroidDemo](https://github.com/rcashie/glTFBastard/blob/master/images/Android.png) 
![WindowsDemo](https://github.com/rcashie/glTFBastard/blob/master/images/Windows.png)

## Example
````c++
std::string source = ReadEntireFile("/Duck/glTF/Duck.gltf");

std::string error;
std::unique_ptr<const glTFBastard::glTF> doc = glTFBastard::Parse(source.c_str(), source.size(), error);

if (!doc) {
  PrintError("glTF load error: %s", error.c_str());
  abort();
}
````

## Features still to be implemented.
* Animation parsing.
* Asset parsing.
* Technique states.
