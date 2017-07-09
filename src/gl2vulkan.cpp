#include "gl2vulkan.h"

unsigned int AttributeGL2Vulkan(int GLdef) {
	int ret = -1;
	switch (GLdef) {
	case GL_FLOAT:
		ret = VK_FORMAT_R32_SFLOAT;
		break;
	case GL_FLOAT_VEC2:
		ret = VK_FORMAT_R32G32_SFLOAT;
		break;
	case GL_FLOAT_VEC3:
		ret = VK_FORMAT_R32G32B32_SFLOAT;
		break;
	case GL_FLOAT_VEC4:
		ret = VK_FORMAT_R32G32B32A32_SFLOAT;
		break;
	case GL_DOUBLE:
		ret = VK_FORMAT_R64_SFLOAT;
		break;
	case GL_DOUBLE_VEC2:
		ret = VK_FORMAT_R64G64_SFLOAT;
		break;
	case GL_DOUBLE_VEC3:
		ret = VK_FORMAT_R64G64B64_SFLOAT;
		break;
	case GL_DOUBLE_VEC4:
		ret = VK_FORMAT_R64G64B64A64_SFLOAT;
		break;
	case GL_INT:
		ret = VK_FORMAT_R32_SINT;
		break;
	case GL_INT_VEC2:
		ret = VK_FORMAT_R32G32_SINT;
		break;
	case GL_INT_VEC3:
		ret = VK_FORMAT_R32G32B32_SINT;
		break;
	case GL_INT_VEC4:
		ret = VK_FORMAT_R32G32B32A32_SINT;
		break;
	case GL_UNSIGNED_INT:
		ret = VK_FORMAT_R32_UINT;
		break;
	case GL_UNSIGNED_INT_VEC2:
		ret = VK_FORMAT_R32G32_UINT;
		break;
	case GL_UNSIGNED_INT_VEC3:
		ret = VK_FORMAT_R32G32B32_UINT;
		break;
	case GL_UNSIGNED_INT_VEC4:
		ret = VK_FORMAT_R32G32B32A32_UINT;
		break;
	case GL_INT64_ARB:
		ret = VK_FORMAT_R64_SINT;
		break;
	case GL_INT64_VEC2_ARB:
		ret = VK_FORMAT_R64G64_SINT;
		break;
	case GL_INT64_VEC3_ARB:
		ret = VK_FORMAT_R64G64B64_SINT;
		break;
	case GL_INT64_VEC4_ARB:
		ret = VK_FORMAT_R64G64B64A64_SINT;
		break;
	case GL_UNSIGNED_INT64_ARB:
		ret = VK_FORMAT_R64_UINT;
		break;
	case GL_UNSIGNED_INT64_VEC2_ARB:
		ret = VK_FORMAT_R64G64_UINT;
		break;
	case GL_UNSIGNED_INT64_VEC3_ARB:
		ret = VK_FORMAT_R64G64B64_UINT;
		break;
	case GL_UNSIGNED_INT64_VEC4_ARB:
		ret = VK_FORMAT_R64G64B64A64_UINT;
		break;
	default:
		break;
	}
	return ret;
}