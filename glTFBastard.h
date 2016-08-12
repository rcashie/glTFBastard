/*
Copyright (c) 2016 Ruben Cashie

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation 
files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, 
modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software 
is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE 
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR 
IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef GLTF_BASTARD_H
#define GLTF_BASTARD_H

#include <string>
#include <vector>
#include <unordered_map>

namespace glTFBastard {

	struct Camera {
		enum Type {
			TYPE_PERSPECTIVE,
			TYPE_ORTHOGRAPHIC
		};

		struct Perspective {
			float aspectRatio;
			float yfov;
			float zfar;
			float znear;
		};

		struct Orthographic {
			float xmag;
			float ymag;
			float zfar;
			float znear;
		};

		union Data {
			Perspective perspective;
			Orthographic orthographic;
		};

		Type type;
		Data typeData;

		Camera() :
			type(TYPE_PERSPECTIVE) {
			memset(static_cast<void*>(&typeData), 0, sizeof(typeData));
		}
	};

	struct Buffer {
		enum Type {
			TYPE_ARRAY_BUFFER,
			TYPE_TEXT
		};

		long long byteLength;
		Type type;
		std::string uri;

		Buffer() :
			type(TYPE_ARRAY_BUFFER),
			byteLength(0) {
		}
	};

	struct BufferView {
		enum Target {
			TARGET_OTHER = 0,
			TARGET_ARRAY_BUFFER = 34962,
			TARGET_ELEMENT_ARRAY_BUFFER = 34963
		};

		std::string buffer;
		long long byteLength;
		long long byteOffset;
		Target target;

		BufferView() :
			byteLength(0),
			byteOffset(0),
			target(TARGET_OTHER) {
		}
	};

	struct Accessor {
		enum ComponentType {
			COMPONENT_TYPE_BYTE = 5120,
			COMPONENT_TYPE_UNSIGNED_BYTE = 5121,
			COMPONENT_TYPE_SHORT = 5122,
			COMPONENT_TYPE_UNSIGNED_SHORT = 5123,
			COMPONENT_TYPE_FLOAT = 5126
		};

		enum Type {
			TYPE_SCALAR,
			TYPE_VEC2,
			TYPE_VEC3,
			TYPE_VEC4,
			TYPE_MAT2,
			TYPE_MAT3,
			TYPE_MAT4
		};

		std::string bufferView;
		long long byteOffset;
		long long byteStride;
		long long count;
		ComponentType componentType;
		Type type;
		std::vector<float> min;
		std::vector<float> max;

		Accessor() :
			byteOffset(0),
			byteStride(0),
			count(0),
			componentType(COMPONENT_TYPE_BYTE),
			type(TYPE_SCALAR) {
		}
	};

	struct Mesh {
		struct Primitive {
			enum Mode {
				TYPE_POINTS = 0,
				TYPE_LINES = 1,
				TYPE_LINE_LOOP = 2,
				TYPE_LINE_STRIP = 3,
				TYPE_TRIANGLES = 4,
				TYPE_TRIANGLE_STRIP = 5,
				TYPE_TRIANGLE_FAN = 6
			};

			std::unordered_map<std::string, std::string> attributes;
			std::string indices;
			std::string material;
			Mode mode;

			Primitive():
				mode(TYPE_TRIANGLES) {
			}
		};

		std::vector<std::unique_ptr<Primitive>> primitives;
	};
	
	struct Shader {
		enum Type {
			TYPE_FRAGMENT_SHADER = 35632,
			TYPE_VERTEX_SHADER = 35633
		};

		Type type;
		std::string uri;

		Shader() :
			type(TYPE_FRAGMENT_SHADER) {
		}
	};

	struct Program {
		std::vector<std::string> attributes;
		std::string fragmentShader;
		std::string vertexShader;
	};

	struct ParameterValue {
		enum Type {
			TYPE_UNKNOWN,
			TYPE_NUMBER,
			TYPE_BOOLEAN,
			TYPE_STRING,
			TYPE_NUMBER_ARRAY,
			TYPE_BOOLEAN_ARRAY,
			TYPE_STRING_ARRAY,
		};

		Type type;
		std::vector<float> numberData;
		std::vector<bool> booleanData;
		std::vector<std::string> stringData;

		ParameterValue() :
			type(TYPE_UNKNOWN) {
		}
	};

	struct Technique {
		struct Parameter {
			enum Type {
				TYPE_BYTE = 5120,
				TYPE_UNSIGNED_BYTE = 5121,
				TYPE_SHORT = 5122,
				TYPE_UNSIGNED_SHORT = 5123,
				TYPE_INT = 5124,
				TYPE_UNSIGNED_INT = 5125,
				TYPE_FLOAT = 5126,
				TYPE_FLOAT_VEC2 = 35664,
				TYPE_FLOAT_VEC3 = 35665,
				TYPE_FLOAT_VEC4 = 35666,
				TYPE_INT_VEC2 = 35667,
				TYPE_INT_VEC3 = 35668,
				TYPE_INT_VEC4 = 35669,
				TYPE_BOOL = 35670,
				TYPE_BOOL_VEC2 = 35671,
				TYPE_BOOL_VEC3 = 35672,
				TYPE_BOOL_VEC4 = 35673,
				TYPE_FLOAT_MAT2 = 35674,
				TYPE_FLOAT_MAT3 = 35675,
				TYPE_FLOAT_MAT4 = 35676,
				TYPE_SAMPLER_2D = 35678
			};

			Type type;
			std::string semantic;
			std::string node;
			std::unique_ptr<ParameterValue> value;

			Parameter() :
				type(TYPE_BYTE) {
			}
		};

		std::unordered_map<std::string, std::unique_ptr<Parameter>> parameters;
		std::unordered_map<std::string, std::string> attributes;
		std::unordered_map<std::string, std::string> uniforms;
		std::string program;

		// TODO: States.
	};

	struct Sampler {
		enum FilterType {
			FILTER_TYPE_NEAREST = 9728,
			FILTER_TYPE_LINEAR = 9729,
			FILTER_TYPE_NEAREST_MINMAP_NEAREST = 9984,
			FILTER_TYPE_LINEAR_MINMAP_NEAREST = 9985,
			FILTER_TYPE_NEAREST_MINMAP_LINEAR = 9986,
			FILTER_TYPE_LINEAR_MINMAP_LINEAR = 9987
		};

		enum WrapType {
			WRAP_TYPE_CLAMP_TO_EDGE = 33071,
			WRAP_TYPE_MIRRORED_REPEAT = 33648,
			WRAP_TYPE_REPEAT = 10497
		};

		FilterType magFilter;
		FilterType minFilter;
		WrapType wrapS;
		WrapType wrapT;

		Sampler() :
			magFilter(FILTER_TYPE_LINEAR),
			minFilter(FILTER_TYPE_NEAREST_MINMAP_LINEAR),
			wrapS(WRAP_TYPE_REPEAT),
			wrapT(WRAP_TYPE_REPEAT) {
		}
	};
	struct Material {
		std::string technique;
		std::unordered_map<std::string, std::unique_ptr<ParameterValue>> values;
	};

	struct Image {
		std::string uri;
	};

	struct Texture {
		enum Format {
			FORMAT_ALPHA = 6406,
			FORMAT_RGB = 6407,
			FORMAT_RGBA = 6408,
			FORMAT_LUMINANCE = 6409,
			FORMAT_LUMINANCE_ALPHA = 6410
		};

		enum Type {
			TYPE_UNSIGNED_BYTE = 5121,
			TYPE_UNSIGNED_SHORT_5_6_5 = 33635,
			TYPE_UNSIGNED_SHORT_4_4_4_4 = 32819,
			TYPE_UNSIGNED_SHORT_5_5_5_1 = 32820
		};

		enum Target {
			TARGET_TEXTURE_2D = 3553
		};

		Format format;
		Format internalFormat;
		std::string sampler;
		std::string source;
		Target target;
		Type type;

		Texture() :
			type(TYPE_UNSIGNED_BYTE),
			target(TARGET_TEXTURE_2D),
			internalFormat(FORMAT_RGBA),
			format(FORMAT_RGBA) {
		};
	};

	struct Animation {
		struct Channel {
			struct Target {
				std::string id;
				std::string path;
			};

			std::string sampler;
			Target target;
		};

		struct Sampler {
			enum Interpolation {
				INTERPOLATION_LINEAR
			};

			Interpolation interpolation;
			std::string input;
			std::string output;

			Sampler() :
				interpolation(INTERPOLATION_LINEAR) {
			}
		};

		std::vector<std::unique_ptr<Channel>> channels;
		std::unordered_map<std::string, std::string> parameters;
		std::unordered_map<std::string, std::unique_ptr<Sampler>> samplers;
	};
	
	struct Skin {
		float bindShapeMatrix[16];
		std::string inverseBindMatrices;
		std::vector<std::string> jointNames;
	};

	struct Node {
		enum TransformType {
			TRANSFORM_TYPE_MATRIX,
			TRANSFORM_TYPE_COMPOSITE
		};

		struct Composite {
			float rotation[4];
			float scale[3];
			float translation[3];
		};

		union Transform {
			Composite composite;
			float matrix[16];
		};
		
		std::string camera;
		std::string skin;
		std::vector<std::string> children;
		std::vector<std::string> skeletons;
		std::vector<std::string> meshes;
		std::string jointName;
		Transform transform;
		TransformType transformType;

		Node() :
			transformType(TRANSFORM_TYPE_MATRIX) {
			memset(static_cast<void*>(&transform), 0, sizeof(transform));
		};
	};

	struct Scene {
		std::vector<std::string> nodes;
	};

	struct glTF {
		std::unordered_map<std::string, std::unique_ptr<Camera>> cameras;
		std::unordered_map<std::string, std::unique_ptr<Buffer>> buffers;
		std::unordered_map<std::string, std::unique_ptr<BufferView>> bufferViews;
		std::unordered_map<std::string, std::unique_ptr<Accessor>> accessors;
		std::unordered_map<std::string, std::unique_ptr<Mesh>> meshes;
		std::unordered_map<std::string, std::unique_ptr<Shader>> shaders;
		std::unordered_map<std::string, std::unique_ptr<Program>> programs;
		std::unordered_map<std::string, std::unique_ptr<Material>> materials;
		std::unordered_map<std::string, std::unique_ptr<Technique>> techniques;
		std::unordered_map<std::string, std::unique_ptr<Sampler>> samplers;
		std::unordered_map<std::string, std::unique_ptr<Texture>> textures;
		std::unordered_map<std::string, std::unique_ptr<Image>> images;
		std::unordered_map<std::string, std::unique_ptr<Animation>> animations;
		std::unordered_map<std::string, std::unique_ptr<Skin>> skins;
		std::unordered_map<std::string, std::unique_ptr<Node>> nodes;
		std::unordered_map<std::string, std::unique_ptr<Scene>> scenes;
		std::string scene;
	};

	std::unique_ptr<const glTF> Parse(const char* jsonString, size_t size, std::string& outErr);
}

#endif