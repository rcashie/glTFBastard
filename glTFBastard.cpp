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

#include <memory>
#include <sstream>
#include "json-parser/json.h"
#include "glTFBastard.h"

namespace glTFBastard {

	// Declaration of a templated function responsible for parsing a json element into it's respective type.
	template<typename T> bool ParseElement(
		const json_value& jsonElement,
		const std::string& elementName,
		T* out,
		std::string& outErr);

	// Parses an array of elements of the specified type.
	template<typename T> bool ParseElement(
		const json_value& jsonElement,
		const std::string& elementName,
		std::vector<T>* outArray,
		std::string& outErr) {

		if (jsonElement.type != json_array) {
			outErr = "Could not parse element '" + elementName + "' as an array.";
			return false;
		}

		int count = jsonElement.u.array.length;
		outArray->clear();
		outArray->reserve(count);

		auto elements = jsonElement.u.array.values;
		for (int i = 0; i < count; ++i) {
			std::stringstream ss;
			ss << elementName << "[" << i << "]";

			T result;
			if (!ParseElement(*elements[i], ss.str(), &result, outErr)) {
				return false;
			}

			outArray->push_back(std::move(result));
		}

		return true;
	}

	// Parses child elements of a json element that are of the same type.
	template<typename T> bool ParseElement(
		const json_value& jsonElement,
		const std::string& elementName,
		std::unordered_map<std::string, T>* outMap,
		std::string& outErr) {

		if (jsonElement.type != json_object) {
			outErr = "Could not parse children of element '" + elementName + "'. It is not an object.";
			return false;
		}

		int childCount = jsonElement.u.object.length;
		outMap->clear();
		outMap->reserve(childCount);

		auto children = jsonElement.u.object.values;
		for (int i = 0; i < childCount; ++i) {
			auto child = children[i];

			T result;
			std::string childName(child.name, child.name_length);
			if (!ParseElement(*child.value, elementName + "." + childName, &result, outErr)) {
				return false;
			}

			outMap->insert(
				std::pair<std::string, T>(childName, std::move(result)));
		}

		return true;
	}

	// Verifies that the element exists before parsing it into it's respective type.
	template<typename T> inline bool ParseRequiredElement(
		const json_value& jsonElement,
		const std::string& elementName,
		T* out,
		std::string& outErr) {

		if (jsonElement.type == json_none) {
			outErr = "The required element '" + elementName + "' does not exist.";
			return false;
		}

		return ParseElement(jsonElement, elementName, out, outErr);
	}

	// Parses an element into it's respective type only if it exists; returns true if the element does not exist.
	template<typename T> inline bool ParseOptionalElement(
		const json_value& jsonElement,
		const std::string& elementName,
		T* out,
		std::string& outErr) {

		// Just return true if it does exist.
		if (jsonElement.type == json_none) {
			return true;
		}

		return ParseElement(jsonElement, elementName, out, outErr);
	}

	// Parses an element only if it exists; returns true if the element does not exist.
	// It then maps the parsed result to another value using the specified map.
	// Handy for validating input against acceptable values.
	template<typename T, typename K> bool ParseAndMapOptionalElement(
		const json_value& jsonElement,
		const std::string& elementName,
		const std::unordered_map<K, T>& map,
		T* out,
		std::string& outErr) {

		// Just return true if it does exist.
		if (jsonElement.type == json_none) {
			return true;
		}

		K elementValue;
		if (!ParseElement(jsonElement, elementName, &elementValue, outErr)) {
			return false;
		}

		auto itr = map.find(elementValue);
		if (itr == map.end()) {
			std::stringstream ss;
			ss << "Unexpected value '" << elementValue << "' for element '" << elementName << "'.";
			outErr = ss.str();
			return false;
		}

		*out = itr->second;
		return true;
	}

	// Verifies that the element exists before parsing an element.
	// It then maps the parsed result to another value using the specified map.
	// Handy for validating input against acceptable values.
	template<typename T, typename K> bool ParseAndMapRequiredElement(
		const json_value& jsonElement,
		const std::string& elementName,
		const std::unordered_map<K, T>& map,
		T* out,
		std::string& outErr) {

		K elementValue;
		if (!ParseRequiredElement(jsonElement, elementName, &elementValue, outErr)) {
			return false;
		}

		auto itr = map.find(elementValue);
		if (itr == map.end()) {
			std::stringstream ss;
			ss << "Unexpected value '" << elementValue << "' for element '" << elementName << "'.";
			outErr = ss.str();
			return false;
		}

		*out = itr->second;
		return true;
	}

	// Parses a fixed sized array of elements of the specified type.
	template<typename T> bool ParseFixedSizeArrayElement(
		const json_value& jsonElement,
		const std::string& elementName,
		size_t outArraySize,
		T* outArray,
		std::string& outErr) {

		if (jsonElement.type != json_array) {
			outErr = "Could not parse element '" + elementName + "' as an array.";
			return false;
		}

		size_t count = outArraySize < jsonElement.u.array.length ? outArraySize : jsonElement.u.array.length;
		auto elements = jsonElement.u.array.values;
		for (int i = 0; i < count; ++i) {
			std::stringstream ss;
			ss << elementName << "[" << i << "]";

			if (!ParseElement(*elements[i], ss.str(), &outArray[i], outErr)) {
				return false;
			}
		}

		return true;
	}

	// Parses a boolean element.
	template<> bool ParseElement<bool>(
		const json_value& jsonElement,
		const std::string& elementName,
		bool* out,
		std::string& outErr) {

		if (jsonElement.type != json_boolean) {
			outErr = "Could not parse element '" + elementName + "' as a boolean.";
			return false;
		}

		*out = jsonElement.u.boolean != 0;
		return true;
	}

	// Parses a float element.
	template<> bool ParseElement<float>(
		const json_value& jsonElement,
		const std::string& elementName,
		float* out,
		std::string& outErr) {

		if (jsonElement.type == json_double) {
			*out = static_cast<float>(jsonElement.u.dbl);
		}
		else if (jsonElement.type == json_integer) {
			*out = static_cast<float>(jsonElement.u.integer);
		}
		else {
			outErr = "Could not parse element '" + elementName + "' as a float.";
			return false;
		}

		return true;
	}

	// Parses an integer element.
	template<> bool ParseElement<long long>(
		const json_value& jsonElement,
		const std::string& elementName,
		long long* out,
		std::string& outErr) {

		if (jsonElement.type != json_integer) {
			outErr = "Could not parse element '" + elementName + "' as an integer.";
			return false;
		}

		*out = jsonElement.u.integer;
		return true;
	}

	// Parses a string element.
	template<> bool ParseElement<std::string>(
		const json_value& jsonElement,
		const std::string& elementName,
		std::string* out,
		std::string& outErr) {

		if (jsonElement.type != json_string) {
			outErr = "Could not parse element '" + elementName + "' as a string.";
			return false;
		}

		(*out).assign(jsonElement.u.string.ptr, jsonElement.u.string.length);
		return true;
	}

	// Parses a Camera element.
	template<> bool ParseElement<std::unique_ptr<Camera>>(
		const json_value& jsonElement,
		const std::string& elementName,
		std::unique_ptr<Camera>* out,
		std::string& outErr) {

		static const std::unordered_map<std::string, Camera::Type> typeMap = {
			{ "orthographic", Camera::TYPE_ORTHOGRAPHIC },
			{ "perspective", Camera::TYPE_PERSPECTIVE }
		};

		std::unique_ptr<Camera> result(new Camera());
		if (!ParseAndMapRequiredElement(jsonElement["type"], elementName + ".type", typeMap, &result->type, outErr)) {
			return false;
		}

		if (result->type == Camera::TYPE_ORTHOGRAPHIC) {
			auto child = jsonElement["orthographic"];
			auto orthographic = &result->typeData.orthographic;
			if (!ParseRequiredElement(child["xmag"], elementName + "orthographic.xmag", &orthographic->xmag, outErr)) {
				return false;
			}

			if (!ParseRequiredElement(child["ymag"], elementName + "orthographic.ymag", &orthographic->ymag, outErr)) {
				return false;
			}

			if (!ParseRequiredElement(child["zfar"], elementName + "orthographic.zfar", &orthographic->zfar, outErr)) {
				return false;
			}

			if (!ParseRequiredElement(child["znear"], elementName + "orthographic.znear", &orthographic->znear, outErr)) {
				return false;
			}
		}
		else if (result->type == Camera::TYPE_PERSPECTIVE) {
			auto child = jsonElement["perspective"];
			auto perspective = &result->typeData.perspective;
			if (!ParseRequiredElement(child["yfov"], elementName + "perspective.yfov", &perspective->yfov, outErr)) {
				return false;
			}

			if (!ParseRequiredElement(child["zfar"], elementName + "perspective.zfar", &perspective->zfar, outErr)) {
				return false;
			}

			if (!ParseRequiredElement(child["znear"], elementName + "perspective.znear", &perspective->znear, outErr)) {
				return false;
			}

			if (!ParseOptionalElement(child["aspectRatio"], elementName + "perspective.aspectRatio", &perspective->aspectRatio, outErr)) {
				return false;
			}
		}

		*out = std::move(result);
		return true;
	}

	// Parses a Buffer element.
	template<> bool ParseElement<std::unique_ptr<Buffer>>(
		const json_value& jsonElement,
		const std::string& elementName,
		std::unique_ptr<Buffer>* out,
		std::string& outErr) {

		static const std::unordered_map<std::string, Buffer::Type> typeMap = {
			{ "arraybuffer", Buffer::TYPE_ARRAY_BUFFER },
			{ "text", Buffer::TYPE_TEXT }
		};

		std::unique_ptr<Buffer> result(new Buffer());
		if (!ParseRequiredElement(jsonElement["uri"], elementName + ".uri", &result->uri, outErr)) {
			return false;
		}

		if (!ParseOptionalElement(jsonElement["byteLength"], elementName + ".byteLength", &result->byteLength, outErr)) {
			return false;
		}

		if (!ParseAndMapOptionalElement(jsonElement["type"], elementName + ".type", typeMap, &result->type, outErr)) {
			return false;
		}

		*out = std::move(result);
		return true;
	}

	// Parses a BufferView element.
	template<> bool ParseElement<std::unique_ptr<BufferView>>(
		const json_value& jsonElement,
		const std::string& elementName,
		std::unique_ptr<BufferView>* out,
		std::string& outErr) {

		static const std::unordered_map<long long, BufferView::Target> targetMap = {
			{ 34962, BufferView::TARGET_ARRAY_BUFFER },
			{ 34963, BufferView::TARGET_ELEMENT_ARRAY_BUFFER }
		};

		std::unique_ptr<BufferView> result(new BufferView());
		if (!ParseRequiredElement(jsonElement["buffer"], elementName + ".buffer", &result->buffer, outErr)) {
			return false;
		}

		if (!ParseRequiredElement(jsonElement["byteOffset"], elementName + ".byteOffset", &result->byteOffset, outErr)) {
			return false;
		}

		if (!ParseOptionalElement(jsonElement["byteLength"], elementName + ".byteLength", &result->byteLength, outErr)) {
			return false;
		}

		if (!ParseAndMapOptionalElement(
			jsonElement["target"], elementName + ".target", targetMap, &result->target, outErr)) {
			return false;
		}

		*out = std::move(result);
		return true;
	}

	// Parses an Accessor element.
	template<> bool ParseElement<std::unique_ptr<Accessor>>(
		const json_value& jsonElement,
		const std::string& elementName,
		std::unique_ptr<Accessor>* out,
		std::string& outErr) {

		static const std::unordered_map<std::string, Accessor::Type> typeMap = {
			{ "SCALAR", Accessor::TYPE_SCALAR },
			{ "VEC2", Accessor::TYPE_VEC2 },
			{ "VEC3", Accessor::TYPE_VEC3 },
			{ "VEC4", Accessor::TYPE_VEC4 },
			{ "MAT2", Accessor::TYPE_MAT2 },
			{ "MAT3", Accessor::TYPE_MAT3 },
			{ "MAT4", Accessor::TYPE_MAT4 }
		};

		static const std::unordered_map<long long, Accessor::ComponentType> componentTypeMap = {
			{ 5120, Accessor::COMPONENT_TYPE_BYTE },
			{ 5121, Accessor::COMPONENT_TYPE_UNSIGNED_BYTE },
			{ 5122, Accessor::COMPONENT_TYPE_SHORT },
			{ 5123, Accessor::COMPONENT_TYPE_UNSIGNED_SHORT },
			{ 5126, Accessor::COMPONENT_TYPE_FLOAT }
		};

		std::unique_ptr<Accessor> result(new Accessor());
		if (!ParseRequiredElement(jsonElement["bufferView"], elementName + ".bufferView", &result->bufferView, outErr)) {
			return false;
		}

		if (!ParseRequiredElement(jsonElement["byteOffset"], elementName + ".byteOffset", &result->byteOffset, outErr)) {
			return false;
		}

		if (!ParseAndMapRequiredElement(
			jsonElement["componentType"], elementName + ".componentType", componentTypeMap, &result->componentType, outErr)) {
			return false;
		}

		if (!ParseAndMapRequiredElement(jsonElement["type"], elementName + ".type", typeMap, &result->type, outErr)) {
			return false;
		}

		if (!ParseRequiredElement(jsonElement["count"], elementName + ".count", &result->count, outErr)) {
			return false;
		}

		if (!ParseOptionalElement(jsonElement["byteStride"], elementName + ".byteStride", &result->byteStride, outErr)) {
			return false;
		}

		if (!ParseOptionalElement(jsonElement["min"], elementName + ".min", &result->min, outErr)) {
			return false;
		}

		if (!ParseOptionalElement(jsonElement["max"], elementName + ".max", &result->max, outErr)) {
			return false;
		}

		*out = std::move(result);
		return true;
	}

	// Parses a Mesh::Primitive element.
	template<> bool ParseElement<std::unique_ptr<Mesh::Primitive>>(
		const json_value& jsonElement,
		const std::string& elementName,
		std::unique_ptr<Mesh::Primitive>* out,
		std::string& outErr) {

		static const std::unordered_map<long long, Mesh::Primitive::Mode> modeMap = {
			{ 0, Mesh::Primitive::TYPE_POINTS },
			{ 1, Mesh::Primitive::TYPE_LINES },
			{ 2, Mesh::Primitive::TYPE_LINE_LOOP },
			{ 3, Mesh::Primitive::TYPE_LINE_STRIP },
			{ 4, Mesh::Primitive::TYPE_TRIANGLES },
			{ 5, Mesh::Primitive::TYPE_TRIANGLE_STRIP },
			{ 6, Mesh::Primitive::TYPE_TRIANGLE_FAN }
		};

		std::unique_ptr<Mesh::Primitive> result(new Mesh::Primitive());
		if (!ParseOptionalElement(jsonElement["attributes"], elementName + ".attributes", &result->attributes, outErr)) {
			return false;
		}

		if (!ParseOptionalElement(jsonElement["indices"], elementName + ".indices", &result->indices, outErr)) {
			return false;
		}

		if (!ParseRequiredElement(jsonElement["material"], elementName + ".material", &result->material, outErr)) {
			return false;
		}

		if (!ParseAndMapOptionalElement(jsonElement["mode"], elementName + ".indicies", modeMap, &result->mode, outErr)) {
			return false;
		}

		*out = std::move(result);
		return true;
	}

	// Parses a Mesh element.
	template<> bool ParseElement<std::unique_ptr<Mesh>>(
		const json_value& jsonElement,
		const std::string& elementName,
		std::unique_ptr<Mesh>* out,
		std::string& outErr) {

		std::unique_ptr<Mesh> result(new Mesh());
		if (!ParseOptionalElement(jsonElement["primitives"], elementName + ".primitives", &result->primitives, outErr)) {
			return false;
		}

		*out = std::move(result);
		return true;
	}

	// Parses a Shader element.
	template<> bool ParseElement<std::unique_ptr<Shader>>(
		const json_value& jsonElement,
		const std::string& elementName,
		std::unique_ptr<Shader>* out,
		std::string& outErr) {

		static const std::unordered_map<long long, Shader::Type> typeMap = {
			{ 35632, Shader::TYPE_FRAGMENT_SHADER },
			{ 35633, Shader::TYPE_VERTEX_SHADER }
		};

		std::unique_ptr<Shader> result(new Shader());
		if (!ParseRequiredElement(jsonElement["uri"], elementName + ".uri", &result->uri, outErr)) {
			return false;
		}

		if (!ParseAndMapRequiredElement(jsonElement["type"], elementName + ".type", typeMap, &result->type, outErr)) {
			return false;
		}

		*out = std::move(result);
		return true;
	}

	// Parses a Program element.
	template<> bool ParseElement<std::unique_ptr<Program>>(
		const json_value& jsonElement,
		const std::string& elementName,
		std::unique_ptr<Program>* out,
		std::string& outErr) {

		std::unique_ptr<Program> result(new Program());
		if (!ParseOptionalElement(jsonElement["attributes"], elementName + ".attributes", &result->attributes, outErr)) {
			return false;
		}

		if (!ParseRequiredElement(jsonElement["fragmentShader"], elementName + ".fragmentShader", &result->fragmentShader, outErr)) {
			return false;
		}

		if (!ParseRequiredElement(jsonElement["vertexShader"], elementName + ".vertexShader", &result->vertexShader, outErr)) {
			return false;
		}

		*out = std::move(result);
		return true;
	}

	// Parses a Parameter Value element.
	template<> bool ParseElement<std::unique_ptr<ParameterValue>>(
		const json_value& jsonElement,
		const std::string& elementName,
		std::unique_ptr<ParameterValue>* out,
		std::string& outErr) {

		std::unique_ptr<ParameterValue> result(new ParameterValue());
		if (jsonElement.type == json_array) {
			// Assume the type of array based on the first element.
			json_type valueType = jsonElement.u.array.length ? jsonElement.u.array.values[0]->type : json_none;

			switch (valueType) {
				case json_integer: 
				case json_double: {
					if (!ParseElement(jsonElement, elementName, &result->numberData, outErr)) {
						return false;
					}

					result->type = ParameterValue::TYPE_NUMBER_ARRAY;
					break;
				}
				case json_string: {
					if (!ParseElement(jsonElement, elementName, &result->stringData, outErr)) {
						return false;
					}

					result->type = ParameterValue::TYPE_STRING_ARRAY;
					break;
				}
				case json_boolean: {
					if (!ParseElement(jsonElement, elementName, &result->booleanData, outErr)) {
						return false;
					}

					result->type = ParameterValue::TYPE_BOOLEAN_ARRAY;
					break;
				}
				default: {
					outErr = "Could not parse parameter value element '" + elementName + "'. Unsupported array type.";
					return false;
				}
			}
		}
		else {
			switch (jsonElement.type) {
				case json_integer:
				case json_double: {
					float numberValue;
					if (!ParseElement(jsonElement, elementName, &numberValue, outErr)) {
						return false;
					}

					result->type = ParameterValue::TYPE_NUMBER;
					result->numberData.push_back(numberValue);
					break;
				}
				case json_string: {
					std::string stringValue;
					if (!ParseElement(jsonElement, elementName, &stringValue, outErr)) {
						return false;
					}

					result->type = ParameterValue::TYPE_STRING;
					result->stringData.push_back(stringValue);
					break;
				}
				case json_boolean: {
					bool booleanValue;
					if (!ParseElement(jsonElement, elementName, &booleanValue, outErr)) {
						return false;
					}

					result->type = ParameterValue::TYPE_BOOLEAN;
					result->booleanData.push_back(booleanValue);
					break;
				}
				default: {
					outErr = "Could not parse parameter value element '" + elementName + "'. Unsupported type.";
					return false;
				}
			}
		}

		*out = std::move(result);
		return true;
	}

	// Parses a Technique::Parameter element.
	template<> bool ParseElement<std::unique_ptr<Technique::Parameter>>(
		const json_value& jsonElement,
		const std::string& elementName,
		std::unique_ptr<Technique::Parameter>* out,
		std::string& outErr) {

		static const std::unordered_map<long long, Technique::Parameter::Type> typeMap = {
			{ 5120, Technique::Parameter::TYPE_BYTE },
			{ 5121, Technique::Parameter::TYPE_UNSIGNED_BYTE },
			{ 5122, Technique::Parameter::TYPE_SHORT },
			{ 5123, Technique::Parameter::TYPE_UNSIGNED_SHORT },
			{ 5124, Technique::Parameter::TYPE_INT },
			{ 5125, Technique::Parameter::TYPE_UNSIGNED_INT },
			{ 5126, Technique::Parameter::TYPE_FLOAT },
			{ 35664, Technique::Parameter::TYPE_FLOAT_VEC2 },
			{ 35665, Technique::Parameter::TYPE_FLOAT_VEC3 },
			{ 35666, Technique::Parameter::TYPE_FLOAT_VEC4 },
			{ 35667, Technique::Parameter::TYPE_INT_VEC2 },
			{ 35668, Technique::Parameter::TYPE_INT_VEC3 },
			{ 35669, Technique::Parameter::TYPE_INT_VEC4 },
			{ 35670, Technique::Parameter::TYPE_BOOL },
			{ 35671, Technique::Parameter::TYPE_BOOL_VEC2 },
			{ 35672, Technique::Parameter::TYPE_BOOL_VEC3 },
			{ 35673, Technique::Parameter::TYPE_BOOL_VEC4 },
			{ 35674, Technique::Parameter::TYPE_FLOAT_MAT2 },
			{ 35675, Technique::Parameter::TYPE_FLOAT_MAT3 },
			{ 35676, Technique::Parameter::TYPE_FLOAT_MAT4 },
			{ 35678, Technique::Parameter::TYPE_SAMPLER_2D }
		};

		std::unique_ptr<Technique::Parameter> result(new Technique::Parameter());
		if (!ParseOptionalElement(jsonElement["node"], elementName + ".node", &result->node, outErr)) {
			return false;
		}

		if (!ParseAndMapRequiredElement(jsonElement["type"], elementName + ".type", typeMap, &result->type, outErr)) {
			return false;
		}

		if (!ParseOptionalElement(jsonElement["semantic"], elementName + ".semantic", &result->semantic, outErr)) {
			return false;
		}

		if (!ParseOptionalElement(jsonElement["value"], elementName + ".value", &result->value, outErr)) {
			return false;
		}

		*out = std::move(result);
		return true;
	}

	// Parses a Technique element.
	template<> bool ParseElement<std::unique_ptr<Technique>>(
		const json_value& jsonElement,
		const std::string& elementName,
		std::unique_ptr<Technique>* out,
		std::string& outErr) {

		std::unique_ptr<Technique> result(new Technique());
		if (!ParseOptionalElement(jsonElement["parameters"], elementName + ".parameters", &result->parameters, outErr)) {
			return false;
		}

		if (!ParseOptionalElement(jsonElement["attributes"], elementName + ".attributes", &result->attributes, outErr)) {
			return false;
		}

		if (!ParseOptionalElement(jsonElement["uniforms"], elementName + ".uniforms", &result->uniforms, outErr)) {
			return false;
		}

		if (!ParseRequiredElement(jsonElement["program"], elementName + ".program", &result->program, outErr)) {
			return false;
		}

		// TODO: States.
		*out = std::move(result);
		return true;
	}

	// Parses a Sampler element.
	template<> bool ParseElement<std::unique_ptr<Sampler>>(
		const json_value& jsonElement,
		const std::string& elementName,
		std::unique_ptr<Sampler>* out,
		std::string& outErr) {

		static const std::unordered_map<long long, Sampler::FilterType> magFilterTypeMap = {
			{ 9728, Sampler::FILTER_TYPE_NEAREST },
			{ 9729, Sampler::FILTER_TYPE_LINEAR }
		};

		static const std::unordered_map<long long, Sampler::FilterType> minFilterTypeMap = {
			{ 9728, Sampler::FILTER_TYPE_NEAREST },
			{ 9729, Sampler::FILTER_TYPE_LINEAR },
			{ 9984, Sampler::FILTER_TYPE_NEAREST_MINMAP_NEAREST },
			{ 9985, Sampler::FILTER_TYPE_LINEAR_MINMAP_NEAREST },
			{ 9986, Sampler::FILTER_TYPE_NEAREST_MINMAP_LINEAR },
			{ 9987, Sampler::FILTER_TYPE_LINEAR_MINMAP_LINEAR }
		};

		static const std::unordered_map<long long, Sampler::WrapType> wrapTypeMap = {
			{ 33071, Sampler::WRAP_TYPE_CLAMP_TO_EDGE },
			{ 33648, Sampler::WRAP_TYPE_MIRRORED_REPEAT },
			{ 10497, Sampler::WRAP_TYPE_REPEAT }
		};

		std::unique_ptr<Sampler> result(new Sampler());
		if (!ParseAndMapOptionalElement(jsonElement["magFilter"], elementName + ".magFilter", magFilterTypeMap, &result->magFilter, outErr)) {
			return false;
		}

		if (!ParseAndMapOptionalElement(jsonElement["minFilter"], elementName + ".minFilter", minFilterTypeMap, &result->minFilter, outErr)) {
			return false;
		}

		if (!ParseAndMapOptionalElement(jsonElement["wrapS"], elementName + ".wrapS", wrapTypeMap, &result->wrapS, outErr)) {
			return false;
		}

		if (!ParseAndMapOptionalElement(jsonElement["wrapT"], elementName + ".wrapT", wrapTypeMap, &result->wrapT, outErr)) {
			return false;
		}

		*out = std::move(result);
		return true;
	}

	// Parses a Material element.
	template<> bool ParseElement<std::unique_ptr<Material>>(
		const json_value& jsonElement,
		const std::string& elementName,
		std::unique_ptr<Material>* out,
		std::string& outErr) {

		std::unique_ptr<Material> result(new Material());
		if (!ParseOptionalElement(jsonElement["technique"], elementName + ".technique", &result->technique, outErr)) {
			return false;
		}

		if (!ParseOptionalElement(jsonElement["values"], elementName + ".values", &result->values, outErr)) {
			return false;
		}

		*out = std::move(result);
		return true;
	}

	// Parses an Image element.
	template<> bool ParseElement<std::unique_ptr<Image>>(
		const json_value& jsonElement,
		const std::string& elementName,
		std::unique_ptr<Image>* out,
		std::string& outErr) {

		std::unique_ptr<Image> result(new Image());
		if (!ParseRequiredElement(jsonElement["uri"], elementName + ".uri", &result->uri, outErr)) {
			return false;
		}

		*out = std::move(result);
		return true;
	}

	// Parses a Texture element.
	template<> bool ParseElement<std::unique_ptr<Texture>>(
		const json_value& jsonElement,
		const std::string& elementName,
		std::unique_ptr<Texture>* out,
		std::string& outErr) {

		static const std::unordered_map<long long, Texture::Format> formatMap = {
			{ 6406, Texture::FORMAT_ALPHA },
			{ 6407, Texture::FORMAT_RGB },
			{ 6408, Texture::FORMAT_RGBA },
			{ 6409, Texture::FORMAT_LUMINANCE },
			{ 6410, Texture::FORMAT_LUMINANCE_ALPHA }
		};

		static const std::unordered_map<long long, Texture::Type> typeMap = {
			{ 5121, Texture::TYPE_UNSIGNED_BYTE },
			{ 33635, Texture::TYPE_UNSIGNED_SHORT_5_6_5 },
			{ 32819, Texture::TYPE_UNSIGNED_SHORT_4_4_4_4 },
			{ 32820, Texture::TYPE_UNSIGNED_SHORT_5_5_5_1 }
		};

		std::unique_ptr<Texture> result(new Texture());
		if (!ParseRequiredElement(jsonElement["sampler"], elementName + ".sampler", &result->sampler, outErr)) {
			return false;
		}

		if (!ParseRequiredElement(jsonElement["source"], elementName + ".source", &result->source, outErr)) {
			return false;
		}

		if (!ParseAndMapOptionalElement(jsonElement["format"], elementName + ".format", formatMap, &result->format, outErr)) {
			return false;
		}

		if (!ParseAndMapOptionalElement(
			jsonElement["internalFormat"], elementName + ".internalFormat", formatMap, &result->internalFormat, outErr)) {
			return false;
		}

		if (!ParseAndMapOptionalElement(jsonElement["type"], elementName + ".type", typeMap, &result->type, outErr)) {
			return false;
		}

		*out = std::move(result);
		return true;
	}

	// Parses a Skin element.
	template<> bool ParseElement<std::unique_ptr<Skin>>(
		const json_value& jsonElement,
		const std::string& elementName,
		std::unique_ptr<Skin>* out,
		std::string& outErr) {

		// Default value for the bindShapeMatrix;
		static const float defaultBindShapeMatrix[] 
			= { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };

		std::unique_ptr<Skin> result(new Skin());

		// Bind shape matrix.
		auto bindShapeMatrixElement = jsonElement["bindShapeMatrix"];
		if (bindShapeMatrixElement.type == json_none) {
			memcpy(
				static_cast<void*>(&result->bindShapeMatrix[0]),
				static_cast<const void*>(&defaultBindShapeMatrix[0]),
				sizeof(defaultBindShapeMatrix));
		}
		else if (!ParseFixedSizeArrayElement(
			bindShapeMatrixElement,
			elementName + ".bindShapeMatix",
			sizeof(result->bindShapeMatrix) / sizeof(result->bindShapeMatrix[0]),
			&result->bindShapeMatrix[0],
			outErr)) {
			return false;
		}

		if (!ParseRequiredElement(jsonElement["inverseBindMatrices"], elementName + ".inverseBindMatrices", &result->inverseBindMatrices, outErr)) {
			return false;
		}

		if (!ParseRequiredElement(jsonElement["jointNames"], elementName + ".jointNames", &result->jointNames, outErr)) {
			return false;
		}

		*out = std::move(result);
		return true;
	}

	// Parses a Node element.
	template<> bool ParseElement<std::unique_ptr<Node>>(
		const json_value& jsonElement,
		const std::string& elementName,
		std::unique_ptr<Node>* out,
		std::string& outErr) {

		// Default values for the transform properties.
		static const float defaultRotation[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		static const float defaultScale[] = { 1.0f, 1.0f, 1.0f };
		static const float defaultTranslation[] = { 0.0f, 0.0f, 0.0f };
		static const float defaultMatrix[] 
			= { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };

		std::unique_ptr<Node> result(new Node());
		if (!ParseOptionalElement(jsonElement["camera"], elementName + ".camera", &result->camera, outErr)) {
			return false;
		}

		if (!ParseOptionalElement(jsonElement["children"], elementName + ".children", &result->children, outErr)) {
			return false;
		}

		if (!ParseOptionalElement(jsonElement["skeletons"], elementName + ".skeletons", &result->skeletons, outErr)) {
			return false;
		}

		if (!ParseOptionalElement(jsonElement["skin"], elementName + ".skin", &result->skin, outErr)) {
			return false;
		}

		if (!ParseOptionalElement(jsonElement["jointName"], elementName + ".jointName", &result->jointName, outErr)) {
			return false;
		}

		if (!ParseOptionalElement(jsonElement["meshes"], elementName + ".meshes", &result->meshes, outErr)) {
			return false;
		}

		// Parse the node transform.
		// If any one of these elements are set we assume it's a component-based transform.
		auto rotationElement = jsonElement["rotation"];
		auto scaleElement = jsonElement["scale"];
		auto transElement = jsonElement["translation"];
		if (rotationElement.type != json_none
			|| scaleElement.type != json_none
			|| transElement.type != json_none) {

			result->transformType = Node::TRANSFORM_TYPE_COMPOSITE;
			auto composite = &result->transform.composite;

			// Rotation.
			if (rotationElement.type == json_none) {
				memcpy(static_cast<void*>(&composite->rotation[0]), static_cast<const void*>(&defaultRotation[0]), sizeof(defaultRotation));
			}
			else if (!ParseFixedSizeArrayElement(
					rotationElement,
					elementName + ".rotation",
					sizeof(composite->rotation) / sizeof(composite->rotation[0]),
					&composite->rotation[0],
					outErr)) {
				return false;
			}

			// Scale.
			if (scaleElement.type == json_none) {
				memcpy(static_cast<void*>(&composite->scale[0]), static_cast<const void*>(&defaultScale[0]), sizeof(defaultScale));
			}
			else if (!ParseFixedSizeArrayElement(
					scaleElement,
					elementName + ".scale",
					sizeof(composite->scale) / sizeof(composite->scale[0]),
					&composite->scale[0],
					outErr)) {
				return false;
			}

			// Translation.
			if (transElement.type == json_none) {
				memcpy(static_cast<void*>(&composite->translation[0]), static_cast<const void*>(&defaultTranslation[0]), sizeof(defaultTranslation));
			}
			else if (!ParseFixedSizeArrayElement(
					transElement,
					elementName + ".translation",
					sizeof(composite->translation) / sizeof(composite->translation[0]),
					&composite->translation[0],
					outErr)) {
				return false;
			}
		}
		else {
			// Otherwise we assume it's a matrix.
			result->transformType = Node::TRANSFORM_TYPE_MATRIX;
			auto matrixElement = jsonElement["matrix"];

			if (matrixElement.type == json_none) {
				memcpy(static_cast<void*>(&result->transform.matrix[0]), static_cast<const void*>(&defaultMatrix[0]), sizeof(defaultMatrix));
			}
			else if (!ParseFixedSizeArrayElement(
				matrixElement,
				elementName + ".matrix",
				sizeof(result->transform.matrix) / sizeof(result->transform.matrix[0]),
				&result->transform.matrix[0],
				outErr)) {
				return false;
			}
		}

		*out = std::move(result);
		return true;
	}

	// Parses a Scene element.
	template<> bool ParseElement<std::unique_ptr<Scene>>(
		const json_value& jsonElement,
		const std::string& elementName,
		std::unique_ptr<Scene>* out,
		std::string& outErr) {

		std::unique_ptr<Scene> result(new Scene());
		if (!ParseOptionalElement(jsonElement["nodes"], elementName + ".scenes", &result->nodes, outErr)) {
			return false;
		}

		*out = std::move(result);
		return true;
	}

	// Parses an entire glTF json document.
	std::unique_ptr<const glTF> Parse(const char* jsonString, size_t size, std::string& outErr) {
		// Parse the json string.
		json_value* rootElement;
		{
			char parseError[128];
			json_settings settings = { 0 };
			rootElement = json_parse_ex(
				&settings,
				static_cast<const json_char*>(jsonString),
				size,
				parseError);

			if (!rootElement) {
				outErr.assign(parseError);
				return nullptr;
			}
		}

		std::unique_ptr<glTF> result(new glTF());
		if (!ParseOptionalElement((*rootElement)["cameras"], "glTF.cameras", &result->cameras, outErr)) {
			return nullptr;
		}

		if (!ParseOptionalElement((*rootElement)["buffers"], "glTF.buffers", &result->buffers, outErr)) {
			return nullptr;
		}

		if (!ParseOptionalElement((*rootElement)["bufferViews"], "glTF.bufferViews", &result->bufferViews, outErr)) {
			return nullptr;
		}

		if (!ParseOptionalElement((*rootElement)["accessors"], "glTF.accessors", &result->accessors, outErr)) {
			return nullptr;
		}

		if (!ParseOptionalElement((*rootElement)["meshes"], "glTF.meshes", &result->meshes, outErr)) {
			return nullptr;
		}

		if (!ParseOptionalElement((*rootElement)["shaders"], "glTF.shaders", &result->shaders, outErr)) {
			return nullptr;
		}

		if (!ParseOptionalElement((*rootElement)["programs"], "glTF.programs", &result->programs, outErr)) {
			return nullptr;
		}

		if (!ParseOptionalElement((*rootElement)["materials"], "glTF.materials", &result->materials, outErr)) {
			return nullptr;
		}

		if (!ParseOptionalElement((*rootElement)["techniques"], "glTF.techniques", &result->techniques, outErr)) {
			return nullptr;
		}

		if (!ParseOptionalElement((*rootElement)["samplers"], "glTF.samplers", &result->samplers, outErr)) {
			return nullptr;
		}

		if (!ParseOptionalElement((*rootElement)["images"], "glTF.images", &result->images, outErr)) {
			return nullptr;
		}

		if (!ParseOptionalElement((*rootElement)["textures"], "glTF.textures", &result->textures, outErr)) {
			return nullptr;
		}

		if (!ParseOptionalElement((*rootElement)["skins"], "glTF.skins", &result->skins, outErr)) {
			return nullptr;
		}

		if (!ParseOptionalElement((*rootElement)["nodes"], "glTF.nodes", &result->nodes, outErr)) {
			return nullptr;
		}

		if (!ParseOptionalElement((*rootElement)["scenes"], "glTF.scenes", &result->scenes, outErr)) {
			return nullptr;
		}

		if (!ParseOptionalElement((*rootElement)["scene"], "glTF.scene", &result->scene, outErr)) {
			return nullptr;
		}

		// TODO: Animations.
		return std::move(result);
	}
}