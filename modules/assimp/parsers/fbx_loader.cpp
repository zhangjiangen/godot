#include "fbx_loader.hpp"

// -----------------------------------------------------------------------------
#include "fbx_utils.hpp"
#include "scene/resources/surface_tool.h"
#include <fbxsdk/core/arch/fbxarch.h>

// -----------------------------------------------------------------------------

#include <stdio.h>
#include <cassert>
#include <iostream>
// =============================================================================
namespace Std_utils {
// =============================================================================

/// Pops the ith element by swapping the last element of the vector with it
/// and decrementing the size of the vector
template <class T>
static void pop(std::vector<T> &vec, int i) {
	assert(vec.size() > 0);
	vec[i] = vec[vec.size() - 1];
	vec.pop_back();
}

// -----------------------------------------------------------------------------

/// Concatenate v0 and v1 into v0. their types can be different as long as
/// T0 and T1 are equal in terms of byte size.
template <class T0, class T1>
static void concat(std::vector<T0> &v0, std::vector<T1> &v1) {
	assert(sizeof(T0) == sizeof(T1));
	v0.resize(v0.size() + v1.size());
	const int off = v0.size();
	for (unsigned i = 0; i < v1.size(); i++)
		v0[off + i] = *(reinterpret_cast<T0 *>(&(v1[i])));
}

// -----------------------------------------------------------------------------

/// @return true if v0 and v1 are equal in size and their elements match
template <class T0, class T1>
static bool equal(std::vector<T0> &v0, std::vector<T1> &v1) {
	if (v0.size() != v1.size())
		return false;

	for (unsigned i = 0; i < v1.size(); ++i)
		if (v0[i] != v1[i])
			return false;

	return true;
}

// -----------------------------------------------------------------------------

/// Copy src into dst (dst is resized ). their types can be different as long as
/// T0 and T1 are equal in terms of byte size.
template <class T0, class T1>
static void copy(std::vector<T0> &dst, const std::vector<T1> &src) {
	assert(sizeof(T0) == sizeof(T1));
	dst.resize(src.size());
	for (unsigned i = 0; i < src.size(); i++)
		dst[i] = *(reinterpret_cast<const T0 *>(&(src[i])));
}

// -----------------------------------------------------------------------------

/// Find 'elt' in 'vec'. Search is O(n).
template <class T0, class T1>
static bool exists(const std::vector<T0> &vec, const T1 &elt) {
	for (unsigned i = 0; i < vec.size(); i++)
		if (vec[i] == elt)
			return true;

	return false;
}

// -----------------------------------------------------------------------------

/// Find 'elt' in 'vec'. Search is O(n).
/// @return the index of the first occurence of elt
template <class T0, class T1>
static int find(const std::vector<T0> &vec, const T1 &elt) {
	for (unsigned i = 0; i < vec.size(); i++)
		if (vec[i] == elt)
			return i;

	return -1;
}

// -----------------------------------------------------------------------------

/// Erase an element at the 'ith' position.
template <class T0>
static void erase(std::vector<T0> &vec, int ith) {
	vec.erase(vec.begin() + ith);
}

// -----------------------------------------------------------------------------

/// Entirely fill "vec" with "val"
template <class T0>
static void fill(std::vector<T0> &vec, const T0 &val) {
	for (unsigned i = 0; i < vec.size(); ++i)
		vec[i] = val;
}

// -----------------------------------------------------------------------------

/**
 * flatten a two dimensionnal vector "to_flatten" by concatenating every
 * elements in a one dimensional vector "flat".
 * @param [in] to_flatten : vector to be flatten
 * @param [out] flat : the flatten values. It simply the concatenation of every
 * vectors: flat[] = {to_flatten[0] , ... , to_flatten[to_flatten.size - 1]}
 * @param [out] flat_offset : a table of indirections to access values in "flat".
 * @li flat_offset[i*2 + 0] == to_flatten[i][0]
 * @li flat_offset[i*2 + 1] == to_flatten[i].size()
 * @li to_flatten.size() == flat_offset.size() * 2
 * @li to_flatten[i][j] == flat[ flat_offset[i*2 + 0] + j ]
 *
 * Two dimensional look up using "flat" and "flat_offset":
 @code
   for(int i = 0; i < (flat_offset.size() / 2); i++){
      int off     = flat_offset[i*2    ]; // Offset in "flat"
      int nb_elts = flat_offset[i*2 + 1]; // Nb elements of the second dimension
      for(int n = off; n < (off+nb_elts); n++) {
          T elt = flat[ n ];
      }
  }
  @endcode
*/
template <class T>
static void flatten(const std::vector<std::vector<T>> &to_flatten,
		std::vector<T> &flat,
		std::vector<int> &flat_offset) {
	int total = 0;
	for (unsigned i = 0; i < to_flatten.size(); ++i)
		total += to_flatten[i].size();

	flat.resize(total);
	flat_offset.resize(to_flatten.size() * 2);

	int k = 0;
	for (unsigned i = 0; i < to_flatten.size(); i++) {
		int size = (int)to_flatten[i].size();
		flat_offset[2 * i] = k;
		flat_offset[2 * i + 1] = size;
		// Concatenate values
		for (int j = 0; j < size; j++)
			flat[k++] = to_flatten[i][j];
	}
}

// -----------------------------------------------------------------------------

/**
 * flatten a two dimensionnal vector "to_flatten" by concatenating every
 * elements in a one dimensional vector "flat".
 * @param [in] to_flatten : vector to be flatten
 * @param [out] flat : the flatten values. It simply the concatenation of every
 * vectors: flat[] = {to_flatten[0] , ... , to_flatten[to_flatten.size - 1]}
*/
template <class T>
static void flatten(const std::vector<std::vector<T>> &to_flatten,
		std::vector<T> &flat) {
	int total = 0;
	for (unsigned i = 0; i < to_flatten.size(); ++i)
		total += to_flatten[i].size();

	flat.resize(total);

	int k = 0;
	for (unsigned i = 0; i < to_flatten.size(); i++) {
		int size = (int)to_flatten[i].size();
		// Concatenate values
		for (int j = 0; j < size; j++)
			flat[k++] = to_flatten[i][j];
	}
}

} // namespace Std_utils
namespace Std_utils {
// =============================================================================

/// Find and retreive an element from the map. If not found an assertion is
/// triggered
/// @param elt : the element to be found
/// @return what is associated with 'k' in 'map'.
// third template parameter is here to avoid ambiguities
template <class Key, class Elt, class PKey>
static const Elt &find(const std::map<Key, Elt> &map, const PKey &k) {
	typename std::map<Key, Elt>::const_iterator it = map.find(k);
	if (it != map.end())
		return it->second;
	else {
		assert(false);
		return map.begin()->second;
	}
}

// -----------------------------------------------------------------------------

/// Find and retreive an element from the map. If not found an assertion is
/// triggered
/// @param elt : the element to be found
/// @return what is associated with 'k' in 'map'.
// third template parameter is here to avoid ambiguities
template <class Key, class Elt, class PKey>
static Elt &find(std::map<Key, Elt> &map, const PKey &k) {
	typename std::map<Key, Elt>::iterator it = map.find(k);
	if (it != map.end())
		return it->second;
	else {
		assert(false);
		return map.begin()->second;
	}
}

// -----------------------------------------------------------------------------

/// Find and retreive an element from the map.
/// @param elt : the key element to found
/// @param res : what is associated with 'elt' in 'map'.
/// @return if we found the element
// third/fourth templates parameters are there to avoid ambiguities
template <class Key, class Elt, class PKey, class PElt>
static bool find(const std::map<Key, Elt> &map, const PKey &elt, PElt const *&res) {
	typename std::map<Key, Elt>::const_iterator it = map.find(elt);
	if (it != map.end()) {
		res = &(it->second);
		return true;
	} else {
		res = 0;
		return false;
	}
}

// -----------------------------------------------------------------------------

/// Find and retreive an element from the map.
/// @param elt : the key element to found
/// @param res : what is associated with 'elt' in 'map'.
/// @return if we found the element
// third/fourth templates parameters are there to avoid ambiguities
template <class Key, class Elt, class PKey, class PElt>
static bool find(std::map<Key, Elt> &map, const PKey &elt, PElt *&res) {
	typename std::map<Key, Elt>::iterator it = map.find(elt);
	if (it != map.end()) {
		res = &(it->second);
		return true;
	} else {
		res = 0;
		return false;
	}
}

// -----------------------------------------------------------------------------

/// Test if an element is in the map.
/// @param elt : the key element to found
/// @return if we found the element
// third template parameter is here to avoid ambiguities
template <class Key, class Elt, class PKey>
static bool exists(const std::map<Key, Elt> &map, const PKey &elt) {
	return map.find(elt) != map.end();
}

} // namespace Std_utils

// =============================================================================
namespace Loader {
// =============================================================================

/// global variables for the FbxSdk resources manager
FbxManager *g_FBXSdkManager = 0;

// -----------------------------------------------------------------------------

void init_fbx_sdk() {
	// init fbx
	g_FBXSdkManager = FbxManager::Create();
	if (!g_FBXSdkManager) {
		FBXSDK_printf("Error: Unable to create FBX Manager!\n");
		assert(false);
	} else
		FBXSDK_printf("Autodesk FBX SDK version %s\n", g_FBXSdkManager->GetVersion());

	// create an IOSettings object
	FbxIOSettings *ios = FbxIOSettings::Create(g_FBXSdkManager, IOSROOT);
	g_FBXSdkManager->SetIOSettings(ios);

	// Load plugins from the executable directory
}

// -----------------------------------------------------------------------------

void clean_fbx_sdk() {
	if (g_FBXSdkManager != 0)
		g_FBXSdkManager->Destroy();
	g_FBXSdkManager = 0;
}

//// Mesh import utilities =====================================================

void importNormals_byControlPoint(FbxMesh *fbx_mesh,
		FbxGeometryElementNormal *elt_normal,
		Abs_mesh &mesh,
		std::map<std::pair<int, int>, int> &idx_normals,
		int v_size) {
	FbxVector4 n;
	int n_size = mesh._normals.size();
	mesh._normals.resize(n_size + elt_normal->GetDirectArray().GetCount());
	if (elt_normal->GetReferenceMode() == FbxGeometryElement::eDirect) {
		for (int i = 0; i < fbx_mesh->GetControlPointsCount(); ++i) {
			n = elt_normal->GetDirectArray().GetAt(i);
			mesh._normals[n_size + i] = Fbx_utils::to_lnormal(n);
			idx_normals[std::pair<int, int>(v_size + i, -1)] = n_size + i;
		}
	} else {
		for (int i = 0; i < fbx_mesh->GetControlPointsCount(); ++i) {
			n = elt_normal->GetDirectArray().GetAt(elt_normal->GetIndexArray().GetAt(i));
			mesh._normals[n_size + i] = Fbx_utils::to_lnormal(n);
			idx_normals[std::pair<int, int>(v_size + i, -1)] = n_size + i;
		}
	}
}

// -----------------------------------------------------------------------------

void importNormals_byPolygonVertex(FbxMesh *fbx_mesh,
		FbxGeometryElementNormal *elt_normal,
		Abs_mesh &mesh,
		std::map<std::pair<int, int>, int> &idx_normals,
		int v_size) {
	FbxVector4 n;
	// map of indices of normals, in order to quickly know if already seen
	std::map<int, int> seenNormals;
	std::map<int, int>::iterator it;
	int lIndexByPolygonVertex = 0;

	// Lookup polygons
	const int nb_polygons = fbx_mesh->GetPolygonCount();
	for (int p = 0; p < nb_polygons; p++) {
		// Lookup polygon vertices
		int lPolygonSize = fbx_mesh->GetPolygonSize(p);
		for (int i = 0; i < lPolygonSize; i++) {
			int lNormalIndex = 0;
			if (elt_normal->GetReferenceMode() == FbxGeometryElement::eDirect)
				lNormalIndex = lIndexByPolygonVertex;
			if (elt_normal->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
				lNormalIndex = elt_normal->GetIndexArray().GetAt(lIndexByPolygonVertex);
			// record the normal if not already seen
			it = seenNormals.find(lNormalIndex);
			if (it == seenNormals.end()) {
				n = elt_normal->GetDirectArray().GetAt(lNormalIndex);
				mesh._normals.push_back(Fbx_utils::to_lnormal(n));
				seenNormals[lNormalIndex] = mesh._normals.size() - 1;
				// record vertice to normal mapping
				idx_normals[std::pair<int, int>(v_size + fbx_mesh->GetPolygonVertex(p, i), p)] = mesh._normals.size() - 1;
			} else
				// record vertice to normal mapping
				idx_normals[std::pair<int, int>(v_size + fbx_mesh->GetPolygonVertex(p, i), p)] = it->second;

			lIndexByPolygonVertex++;
		}
	}
}

//// Mesh import utilities =====================================================

static void importVertexColors_byControlPoint(FbxMesh *fbx_mesh,
		FbxGeometryElementVertexColor *elt_color,
		Abs_mesh &mesh,
		std::map<std::pair<int, int>, int> &idx_colors,
		int v_size) {
	FbxColor n;
	int n_size = mesh._normals.size();
	mesh._normals.resize(n_size + elt_color->GetDirectArray().GetCount());
	if (elt_color->GetReferenceMode() == FbxGeometryElement::eDirect) {
		for (int i = 0; i < fbx_mesh->GetControlPointsCount(); ++i) {
			n = elt_color->GetDirectArray().GetAt(i);
			mesh._colors[n_size + i] = Fbx_utils::to_color(n);
			idx_colors[std::pair<int, int>(v_size + i, -1)] = n_size + i;
		}
	} else {
		for (int i = 0; i < fbx_mesh->GetControlPointsCount(); ++i) {
			n = elt_color->GetDirectArray().GetAt(elt_color->GetIndexArray().GetAt(i));
			mesh._colors[n_size + i] = Fbx_utils::to_color(n);
			idx_colors[std::pair<int, int>(v_size + i, -1)] = n_size + i;
		}
	}
}

// -----------------------------------------------------------------------------

static void importVertexColors_byPolygonVertex(FbxMesh *fbx_mesh,
		FbxGeometryElementVertexColor *elt_color,
		Abs_mesh &mesh,
		std::map<std::pair<int, int>, int> &idx_colors,
		int v_size) {
	FbxColor n;
	// map of indices of normals, in order to quickly know if already seen
	std::map<int, int> seenNormals;
	std::map<int, int>::iterator it;
	int lIndexByPolygonVertex = 0;

	// Lookup polygons
	const int nb_polygons = fbx_mesh->GetPolygonCount();
	for (int p = 0; p < nb_polygons; p++) {
		// Lookup polygon vertices
		int lPolygonSize = fbx_mesh->GetPolygonSize(p);
		for (int i = 0; i < lPolygonSize; i++) {
			int lNormalIndex = 0;
			if (elt_color->GetReferenceMode() == FbxGeometryElement::eDirect)
				lNormalIndex = lIndexByPolygonVertex;
			if (elt_color->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
				lNormalIndex = elt_color->GetIndexArray().GetAt(lIndexByPolygonVertex);
			// record the normal if not already seen
			it = seenNormals.find(lNormalIndex);
			if (it == seenNormals.end()) {
				n = elt_color->GetDirectArray().GetAt(lNormalIndex);
				mesh._colors.push_back(Fbx_utils::to_color(n));
				seenNormals[lNormalIndex] = mesh._normals.size() - 1;
				// record vertice to normal mapping
				idx_colors[std::pair<int, int>(v_size + fbx_mesh->GetPolygonVertex(p, i), p)] = mesh._normals.size() - 1;
			} else
				// record vertice to normal mapping
				idx_colors[std::pair<int, int>(v_size + fbx_mesh->GetPolygonVertex(p, i), p)] = it->second;

			lIndexByPolygonVertex++;
		}
	}
}

// -----------------------------------------------------------------------------

void importTexCoords_byControlPoint(FbxMesh *lMesh,
		FbxGeometryElementUV *elt_UV,
		Abs_mesh &mesh,
		std::map<std::pair<int, int>, int> &idx_UV,
		int v_size, Vector<Vector2> &_texCoords) {
	FbxVector2 uv;
	int nb_uv = _texCoords.size();
	_texCoords.resize(nb_uv + elt_UV->GetDirectArray().GetCount());
	if (elt_UV->GetReferenceMode() == FbxGeometryElement::eDirect) {
		for (int i = 0; i < lMesh->GetControlPointsCount(); ++i) {
			uv = elt_UV->GetDirectArray().GetAt(i);
			_texCoords.set(nb_uv + i, Fbx_utils::to_ltexcoord(uv));
			idx_UV[std::pair<int, int>(v_size + i, -1)] = nb_uv + i;
		}
	} else {
		for (int i = 0; i < lMesh->GetControlPointsCount(); ++i) {
			uv = elt_UV->GetDirectArray().GetAt(elt_UV->GetIndexArray().GetAt(i));
			_texCoords.set(nb_uv + i, Fbx_utils::to_ltexcoord(uv));
			idx_UV[std::pair<int, int>(v_size + i, -1)] = nb_uv + i;
		}
	}
}

// -----------------------------------------------------------------------------

void importTexCoords_byPolygonVertex(FbxMesh *lMesh,
		FbxGeometryElementUV *elt_UV,
		Abs_mesh &mesh,
		std::map<std::pair<int, int>, int> &idx_UV,
		int v_size, Vector<Vector2> &_texCoords) {
	FbxVector2 uv;
	// map of indices of normals, in order to quickly know if already seen
	std::map<int, int> seenCoords;
	std::map<int, int>::iterator it;
	int lIndexByPolygonVertex = 0;
	for (int p = 0; p < lMesh->GetPolygonCount(); p++) {
		int lPolygonSize = lMesh->GetPolygonSize(p);
		for (int i = 0; i < lPolygonSize; i++) {
			int lTexCoordIndex = 0;
			if (elt_UV->GetReferenceMode() == FbxGeometryElement::eDirect)
				lTexCoordIndex = lIndexByPolygonVertex;
			if (elt_UV->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
				lTexCoordIndex = elt_UV->GetIndexArray().GetAt(lIndexByPolygonVertex);

			// record the normal if not already seen
			it = seenCoords.find(lTexCoordIndex);
			if (it == seenCoords.end()) {
				uv = elt_UV->GetDirectArray().GetAt(lTexCoordIndex);
				_texCoords.push_back(Fbx_utils::to_ltexcoord(uv));
				seenCoords[lTexCoordIndex] = _texCoords.size() - 1;
				// record vertice to normal mapping
				idx_UV[std::pair<int, int>(v_size + lMesh->GetPolygonVertex(p, i), p)] = _texCoords.size() - 1;
			} else
				idx_UV[std::pair<int, int>(v_size + lMesh->GetPolygonVertex(p, i), p)] = it->second;

			lIndexByPolygonVertex++;
		}
	}
}

// -----------------------------------------------------------------------------

void fill_material(FbxNode *node, Abs_mesh &mesh) {
	for (int i = 0; i < node->GetMaterialCount(); ++i) {
		Material m;
		// get material infos
		FbxSurfaceMaterial *fbx_mat = node->GetMaterial(i);

		const FbxImplementation *implem = GetImplementation(fbx_mat, FBXSDK_IMPLEMENTATION_HLSL);
		if (implem == 0)
			implem = GetImplementation(fbx_mat, FBXSDK_IMPLEMENTATION_CGFX);

		if (implem != 0)
			std::cerr << "Material implementation not supported : CGFX/HLSL.\n";
		else if (fbx_mat->GetClassId().Is(FbxSurfacePhong::ClassId))
			Fbx_utils::copy(m, (FbxSurfacePhong *)fbx_mat);
		else if (fbx_mat->GetClassId().Is(FbxSurfaceLambert::ClassId))
			Fbx_utils::copy(m, (FbxSurfaceLambert *)fbx_mat);
		else {
			std::cerr << "ERROR FBX : unhandle material type we won't load it";
			std::cerr << std::endl;
		}

		// add material
		if (implem == 0)
			mesh._materials.push_back(m);

	} // for materialCount
}
Ref<SpatialMaterial> fill_material(FbxNode *node) {
	Ref<SpatialMaterial> ret;
	ret.instance();
	for (int i = 0; i < node->GetMaterialCount(); ++i) {
		Material m;
		// get material infos
		FbxSurfaceMaterial *fbx_mat = node->GetMaterial(i);
		ret->set_name(fbx_mat->GetName());

	} // for materialCount
	return ret;
}
// -----------------------------------------------------------------------------

#include "scene/resources/surface_tool.h"
void Parser_FbxMesh(Ref<SurfaceTool> st, FbxMesh *fbx_mesh, bool is_skin,
		Map<int, Pair<Vector<int32_t>, Vector<float_t>>> boneWeight) {
	bool isNormalByControlPoint = true;
	int nb_verts = fbx_mesh->GetControlPointsCount();
	Abs_mesh mesh;
	FbxGeometryElement::EMappingMode type;
	int v_size = mesh._vertices.size();
	std::map<std::pair<int, int>, int> idx_normals;
	// 解析模型
	{
		// vertices ################################################################
		FbxVector4 *v = fbx_mesh->GetControlPoints();
		mesh._vertices.resize(v_size + nb_verts);
		for (int i = 0; i < nb_verts; ++i) {
			mesh._vertices[v_size + i] = Fbx_utils::to_lvertex(v[i]);
		}

		// normals #################################################################

		// map the indice of face and vertice to indice of normal
		FbxGeometryElementNormal *elt_normal;

		int nb_elt_normal = fbx_mesh->GetElementNormalCount();

		if (nb_elt_normal > 1) {
			std::cerr << "WARNING FBX : there is more than one layer for normals";
			std::cerr << "We only handle the first layer" << std::endl;
		}
		if (nb_elt_normal > 0) {
			// Fetch first element
			elt_normal = fbx_mesh->GetElementNormal();
			type = elt_normal->GetMappingMode();

			if (type == FbxGeometryElement::eByControlPoint) {
				importNormals_byControlPoint(fbx_mesh, elt_normal, mesh, idx_normals, v_size);
			} else if (type == FbxGeometryElement::eByPolygonVertex) {
				isNormalByControlPoint = false;
				importNormals_byPolygonVertex(fbx_mesh, elt_normal, mesh, idx_normals, v_size);
			} else {
				std::cerr << "ERROR FBX: mapping mode'" << Fbx_utils::to_string(type).utf8();
				std::cerr << "'for normals is not handled" << std::endl;
			}
		}
		// texCoords ###############################################################
		// map the indice of face and vertice to indice of normal
		FbxGeometryElementUV *elt_uv;

		int nb_elt_uv = fbx_mesh->GetElementUVCount();

		if (nb_elt_uv > 1) {
			std::cerr << "WARNING FBX : there is more than one layer for texture coordinates";
			std::cerr << "We only handle the first layer" << std::endl;
		}
	}

	int nb_elt_uv = fbx_mesh->GetElementUVCount();
	FbxGeometryElementUV *elt_uv;
	Vector<Vector2> uv0;
	Vector<Vector2> uv1;
	Vector<Vector2> uv2;
	Vector<Vector2> uv3;
	Vector<Vector2> uv4;
	bool isUVByControlPoint = true;
	std::map<std::pair<int, int>, int> idx_uv;
	if (nb_elt_uv > 0) {
		// Fetch first element
		elt_uv = fbx_mesh->GetElementUV(0);
		type = elt_uv->GetMappingMode();

		if (type == FbxGeometryElement::eByControlPoint) {
			importTexCoords_byControlPoint(fbx_mesh, elt_uv, mesh, idx_uv, v_size, uv0);
		} else if (type == FbxGeometryElement::eByPolygonVertex) {
			isUVByControlPoint = false;
			importTexCoords_byPolygonVertex(fbx_mesh, elt_uv, mesh, idx_uv, v_size, uv0);
		} else {
			std::cerr << "ERROR FBX: mapping mode'" << Fbx_utils::to_string(type).utf8();
			std::cerr << "'for tex coords is not handled" << std::endl;
		}
	}

	std::map<std::pair<int, int>, int> idx_uv1;
	if (nb_elt_uv > 1) {
		// Fetch first element
		elt_uv = fbx_mesh->GetElementUV(1);

		FbxGeometryElement::EMappingMode type1 = elt_uv->GetMappingMode();

		if (type1 == FbxGeometryElement::eByControlPoint) {
			importTexCoords_byControlPoint(fbx_mesh, elt_uv, mesh, idx_uv1, v_size, uv1);
		} else if (type1 == FbxGeometryElement::eByPolygonVertex) {
			isUVByControlPoint = false;
			importTexCoords_byPolygonVertex(fbx_mesh, elt_uv, mesh, idx_uv1, v_size, uv1);
		} else {
			std::cerr << "ERROR FBX: mapping mode'" << Fbx_utils::to_string(type).utf8();
			std::cerr << "'for tex coords is not handled" << std::endl;
		}
	}
	std::map<std::pair<int, int>, int> idx_uv2;
	if (nb_elt_uv > 2) {
		// Fetch first element
		elt_uv = fbx_mesh->GetElementUV(2);

		FbxGeometryElement::EMappingMode type2 = elt_uv->GetMappingMode();

		if (type2 == FbxGeometryElement::eByControlPoint) {
			importTexCoords_byControlPoint(fbx_mesh, elt_uv, mesh, idx_uv2, v_size, uv2);
		} else if (type2 == FbxGeometryElement::eByPolygonVertex) {
			isUVByControlPoint = false;
			importTexCoords_byPolygonVertex(fbx_mesh, elt_uv, mesh, idx_uv2, v_size, uv2);
		} else {
			std::cerr << "ERROR FBX: mapping mode'" << Fbx_utils::to_string(type).utf8();
			std::cerr << "'for tex coords is not handled" << std::endl;
		}
	}
	std::map<std::pair<int, int>, int> idx_uv3;
	if (nb_elt_uv > 3) {
		// Fetch first element
		elt_uv = fbx_mesh->GetElementUV(3);

		FbxGeometryElement::EMappingMode type3 = elt_uv->GetMappingMode();

		if (type3 == FbxGeometryElement::eByControlPoint) {
			importTexCoords_byControlPoint(fbx_mesh, elt_uv, mesh, idx_uv3, v_size, uv3);
		} else if (type3 == FbxGeometryElement::eByPolygonVertex) {
			isUVByControlPoint = false;
			importTexCoords_byPolygonVertex(fbx_mesh, elt_uv, mesh, idx_uv3, v_size, uv3);
		} else {
			std::cerr << "ERROR FBX: mapping mode'" << Fbx_utils::to_string(type).utf8();
			std::cerr << "'for tex coords is not handled" << std::endl;
		}
	}

	// 获取顶点颜色
	std::map<std::pair<int, int>, int> idx_color;

	if (fbx_mesh->GetElementVertexColorCount()) {
		FbxGeometryElementVertexColor *fbx_color = fbx_mesh->GetElementVertexColor();
		FbxGeometryElement::EMappingMode type1 = elt_uv->GetMappingMode();
		if (type == FbxGeometryElement::eByControlPoint) {
			importVertexColors_byControlPoint(fbx_mesh, fbx_color, mesh, idx_uv, v_size);
		} else if (type == FbxGeometryElement::eByPolygonVertex) {
			isUVByControlPoint = false;
			importVertexColors_byPolygonVertex(fbx_mesh, fbx_color, mesh, idx_uv, v_size);
		} else {
			std::cerr << "ERROR FBX: mapping mode'" << Fbx_utils::to_string(type).utf8();
			std::cerr << "'for tex coords is not handled" << std::endl;
		}
	}
	// triangles ###############################################################
	int f_size = mesh._triangles.size();
	mesh._triangles.resize(f_size + fbx_mesh->GetPolygonCount());
	for (int faceIndex = 0; faceIndex < fbx_mesh->GetPolygonCount(); ++faceIndex) {
		Tri_face f;
		for (int verticeIndex = 0; verticeIndex < fbx_mesh->GetPolygonSize(faceIndex); ++verticeIndex) { // here size = 3
			// register the vertice indice
			f.v[verticeIndex] = v_size + fbx_mesh->GetPolygonVertex(faceIndex, verticeIndex);
			// register the vertice's normal indice
			if (fbx_mesh->GetElementNormalCount())
				f.n[verticeIndex] = idx_normals[std::pair<int, int>(f.v[verticeIndex], isNormalByControlPoint ? -1 : faceIndex)];
			// register the vertice's texcoords indice
			if (fbx_mesh->GetElementUVCount())
				f.t[verticeIndex] = idx_uv[std::pair<int, int>(f.v[verticeIndex], isUVByControlPoint ? -1 : faceIndex)];
		}
		mesh._triangles[f_size + faceIndex] = f;
	}

	st->begin(Mesh::PRIMITIVE_TRIANGLES);
	for (int i = 0; i < mesh._vertices.size(); ++i) {
		if (nb_elt_uv > 0) {
			st->add_uv(uv0[i]);
		}
		if (nb_elt_uv > 1) {
			st->add_uv2(uv1[i]);
		}
		if (nb_elt_uv > 2) {
			st->add_uv3(uv2[i]);
		}
		if (nb_elt_uv > 3) {
			st->add_uv4(uv3[i]);
		}
		if (i < mesh._colors.size()) {
			st->add_color(mesh._colors[i]);
		}
		if (i < mesh._normals.size()) {
			st->add_normal(mesh._normals[i]);
		}
		// 增加骨骼索引信息
		if (is_skin) {
			auto bw = boneWeight.find(i);
			if (bw) {
				st->add_bones(bw->value().first);
				st->add_weights(bw->value().second);
			}
		}
		st->add_vertex(mesh._vertices[i]);
	}
	// 增加顶点索引
	for (int i = 0; i < mesh._triangles.size(); ++i) {
		st->add_index(mesh._triangles[i].v[0]);
		st->add_index(mesh._triangles[i].v[1]);
		st->add_index(mesh._triangles[i].v[2]);
	}
}
struct FBX_LoadContext {
	Ref<ArrayMesh> godot_mesh;
	MeshInstance *mesh_instance;
	// 绑定的骨骼信息
	Ref<Skin> bind_skin;
	Skeleton *skeleton;
};
void fill_mesh(Fbx_file *file, FbxMesh *fbx_mesh, FbxNode *node, Abs_mesh &mesh, FBX_LoadContext &context) {
	// deal with non triangular mesh
	if (!fbx_mesh->IsTriangleMesh()) {
		FbxGeometryConverter triangulator(g_FBXSdkManager);
		//fbx_mesh = triangulator.TriangulateMesh(fbx_mesh);
		FbxNodeAttribute *nodeAttr = triangulator.Triangulate(fbx_mesh, true /*<- replace node*/);
		FbxNode *node = nodeAttr->GetNode();
		if (nodeAttr->GetAttributeType() == FbxNodeAttribute::eMesh)
			fbx_mesh = node->GetMesh();
		else
			assert(false && "ERROR: This is really unexpected...");
	}

	FbxDeformer *Deformer = nullptr;
	Skeleton *_Skelect = nullptr;
	Array morphs;
	FbxNode *Link = nullptr;
	Map<int, Pair<Vector<int32_t>, Vector<float_t>>> BoneWeight;
	for (int def_index = 0; def_index < fbx_mesh->GetDeformerCount(); ++def_index) {
		Deformer = fbx_mesh->GetDeformer(def_index);
		switch (Deformer->GetDeformerType()) {
			case FbxDeformer::eSkin: {
				FbxSkin *skin_def = (FbxSkin *)Deformer;
				int32_t ClusterCount = skin_def->GetClusterCount();
				bool bFoundCorrectLink = false;
				for (int32_t ClusterId = 0; ClusterId < ClusterCount; ++ClusterId) {
					FbxCluster *cluster = skin_def->GetCluster(ClusterId);
					FbxNode *Link = cluster->GetLink(); //Get the bone influences by this first cluster
					// 查找所属的骨架信息
					Skeleton *skin = file->FindSkelect(Link);
					if (skin) {
						_Skelect = skin;
						context.skeleton = skin;
						Ref<Skin> bind_skin = context.mesh_instance->get_skin();
						// 设置蒙皮信息
						if (!bind_skin.is_valid()) {
							bind_skin.instance();
							context.mesh_instance->set_skin(bind_skin);
						}
						bind_skin->add_named_bind(Link->GetName(), skin->get_global_transform().inverse() * Fbx_utils::to_transfo(Link->EvaluateGlobalTransform()).inverse());
					}
					//

					int bone_id = skin->find_bone(Link->GetName());

					const int size_cluster = cluster->GetControlPointIndicesCount();
					for (int c = 0; c < size_cluster; c++) {
						const int fbx_idx = cluster->GetControlPointIndices()[c];
						const float fbx_weight = (float)cluster->GetControlPointWeights()[c];

						BoneWeight[fbx_idx].first.push_back(bone_id);
						BoneWeight[fbx_idx].second.push_back(fbx_weight);
					}
				}

			}
			/* code */
			break;
			case FbxDeformer::eBlendShape: {
				// 动画变形信息
				FbxBlendShape *BlendShape = (FbxBlendShape *)Deformer;
				String BlendShapeName = BlendShape->GetName();
				const int32_t BlendShapeChannelCount = BlendShape->GetBlendShapeChannelCount();
				for (int32_t ChannelIndex = 0; ChannelIndex < BlendShapeChannelCount; ++ChannelIndex) {
					FbxBlendShapeChannel *Channel = BlendShape->GetBlendShapeChannel(ChannelIndex);

					if (Channel) {
						String ChannelName = Channel->GetName();
						FbxShape *shape = Channel->GetTargetShape(ChannelIndex);
						if (shape) {
							shape->GetBaseGeometry();
							FbxGeometry *fbxGeom = shape->GetBaseGeometry();
							if (fbxGeom && fbxGeom->GetAttributeType() == FbxNodeAttribute::eMesh) {
								FbxMesh *meshShape = (FbxMesh *)fbxGeom;
								// 下面解析模型
								Ref<SurfaceTool> morph_st;
								morph_st.instance();
								Parser_FbxMesh(morph_st, meshShape, BoneWeight.size() > 0, BoneWeight);
								morphs.push_back(morph_st->commit_to_arrays());
							}
						}
					}
				}
			}

			break;
			case FbxDeformer::eVertexCache:
				break;
			default:
				break;
		}
	}
	Ref<SurfaceTool> st;
	st.instance();
	st->begin(Mesh::PRIMITIVE_TRIANGLES);

	// materials ###############################################################
	int m_size = mesh._materials.size();
	auto mat = fill_material(node);
	st->set_material(mat);

	// material groups & groups ################################################
	Group g;
	g._start_face = 0;
	g._end_face = mesh._triangles.size();
	int nb_elementMaterial = fbx_mesh->GetElementMaterialCount();
	FbxGeometryElementMaterial *lMaterialElement;
	if (nb_elementMaterial == 1) {
		lMaterialElement = fbx_mesh->GetElementMaterial();
		if (lMaterialElement->GetMappingMode() == FbxGeometryElement::eAllSame) {
			Material_group mg;
			mg._material_idx = m_size + lMaterialElement->GetIndexArray().GetAt(0);
			mg._start_face = 0;
			mg._end_face = mesh._triangles.size();
			g._assigned_mats.push_back(mg);
		} else {
			for (int i = 0; i < fbx_mesh->GetPolygonCount(); ++i) {
				// register all material groups
				Material_group mg;
				mg._material_idx = m_size + lMaterialElement->GetIndexArray().GetAt(i);
				mg._start_face = 0 + i;
				for (; i < fbx_mesh->GetPolygonCount() && (m_size + lMaterialElement->GetIndexArray().GetAt(i) == (int)mg._material_idx); ++i)
					;
				mg._end_face = 0 + i;
				g._assigned_mats.push_back(mg);
				--i;
			}
		}
		mesh._groups.push_back(g);
	} else if (nb_elementMaterial > 1) {
		int i = 0;
		for (int i = 0; i < nb_elementMaterial; ++i) {
			if (fbx_mesh->GetElementMaterial(i)->GetMappingMode() == FbxGeometryElement::eByPolygon)
				break;
		}
		if (i >= nb_elementMaterial) {
			lMaterialElement = fbx_mesh->GetElementMaterial();
			Material_group mg;
			mg._material_idx = m_size + lMaterialElement->GetIndexArray().GetAt(0);
			mg._start_face = 0;
			mg._end_face = mesh._triangles.size();
			g._assigned_mats.push_back(mg);
		} else {
			lMaterialElement = fbx_mesh->GetElementMaterial(i);
			for (int j = 0; j < fbx_mesh->GetPolygonCount(); ++j) {
				// register all material groups
				Material_group mg;
				mg._material_idx = m_size + lMaterialElement->GetIndexArray().GetAt(j);
				mg._start_face = 0 + j;
				for (; j < fbx_mesh->GetPolygonCount() && (m_size + lMaterialElement->GetIndexArray().GetAt(j) == (int)mg._material_idx); ++j)
					;
				mg._end_face = 0 + j;
				g._assigned_mats.push_back(mg);
				--j;
			}
		}
		mesh._groups.push_back(g);
	}
	/////////////////////////////////////////////
	context.godot_mesh->add_surface_from_arrays(
			Mesh::PRIMITIVE_TRIANGLES,
			st->commit_to_arrays(),
			morphs,
			true);
	context.godot_mesh->set_name(fbx_mesh->GetName());
	context.godot_mesh->surface_set_material(0, mat);
}

//------------------------------------------------------------------------------

void Fbx_file::compute_size_mesh() {
	_offset_verts.clear();
	int acc = 0;
	for (int i = 0; i < _fbx_scene->GetNodeCount(); i++) {
		FbxNode *node = _fbx_scene->GetNode(i);
		FbxNodeAttribute *attr = node->GetNodeAttribute();

		if (attr != 0 && attr->GetAttributeType() == FbxNodeAttribute::eMesh) {
			_offset_verts[node] = acc;
			acc += ((FbxMesh *)attr)->GetControlPointsCount();
		}
	}
	_size_mesh = acc;
}

//------------------------------------------------------------------------------

bool Fbx_file::import_file(const String &filename) {
	assert(g_FBXSdkManager != 0);
	Base_loader::update_paths(filename);
	free_mem();
	// Create the entity that will hold the scene.
	_fbx_scene = FbxScene::Create(g_FBXSdkManager, "");

	bool state = Fbx_utils::load_scene(filename, _fbx_scene, g_FBXSdkManager);
	// 计算定点偏移量
	if (state) {
		compute_size_mesh();
		// 创建场景
		RetFBXScene = memnew(Spatial());
	}

	return state;
}
void Fbx_file::LoadScene(FbxScene *fbx_scene, Spatial *root_scene) {
	// 创建场景节点

	FbxNode *RootNode = fbx_scene->GetRootNode();
	FbxNodeInfo RootInfo;
	RootInfo.ObjectName = RootNode->GetName();
	RootInfo.UniqueId = RootNode->GetUniqueID();
	RootInfo.transform = RootNode->EvaluateGlobalTransform();
	RootInfo.RotationPivot = RootNode->RotationPivot.Get();
	RootInfo.ScalePivot = RootNode->ScalingPivot.Get();
	// 转换一下矩阵
	RootInfo.godot_trans = Fbx_utils::to_transfo(RootInfo.transform);

	RootInfo.AttributeUniqueId = 0;

	RootInfo.ParentUniqueId = 0;

	//Add the rootnode to the SceneInfo
	HierarchyInfo.push_back(RootInfo);
	for (int i = 0; i < fbx_scene->GetNodeCount(); ++i) {
		FbxNode *node = fbx_scene->GetNode(i);
		if (node != nullptr) {
			ProcessNode(node, RootNode, root_scene);
		}
	}
	// 加载所有的模型
	Map<FbxNode *, MeshInstance *>::Element *mesh_map_node = AllMesh.front();
	while (mesh_map_node != nullptr) {
		FBX_LoadContext context;
		context.godot_mesh.instance();
		context.mesh_instance = mesh_map_node->value();
		Abs_mesh mesh;
		FbxNodeAttribute *attr = mesh_map_node->key()->GetNodeAttribute();
		fill_mesh(this, (FbxMesh *)attr, mesh_map_node->key(), mesh, context);
		MeshInstance *mi = mesh_map_node->value();
		mi->set_mesh(context.godot_mesh);
		if (context.skeleton) {
			mi->set_skeleton_path(mi->get_path_to(context.skeleton));

			context.skeleton->add_child(mi);
			mi->set_owner(RetFBXScene);
			mi->set_transform(Transform());
		}
		mesh_map_node = mesh_map_node->next();
	}
}
void Fbx_file::ProcessNode(FbxNode *fbx_node, FbxNode *parent_node, Spatial *root) {
	FbxNodeAttribute *attr = fbx_node->GetNodeAttribute();

	FbxNodeInfo ChildInfo;
	ChildInfo.ObjectName = fbx_node->GetName();
	ChildInfo.UniqueId = fbx_node->GetUniqueID();
	ChildInfo.ParentName = parent_node->GetName();
	ChildInfo.ParentUniqueId = parent_node->GetUniqueID();
	ChildInfo.RotationPivot = fbx_node->RotationPivot.Get();
	ChildInfo.ScalePivot = fbx_node->ScalingPivot.Get();
	ChildInfo.transform = fbx_node->EvaluateLocalTransform();
	ChildInfo.NodeType = attr->GetAttributeType();
	ChildInfo.SourceNode = fbx_node;
	// 转换一下矩阵
	ChildInfo.godot_trans = Fbx_utils::to_transfo(ChildInfo.transform);
	Skeleton *skin = nullptr;
	Spatial *curr_node = nullptr;
	bool IsSkelete = false;
	switch (attr->GetAttributeType()) {
		case FbxNodeAttribute::eUnknown: {
			ChildInfo.AttributeType = "eUnknown";
			Spatial *node = memnew(Spatial);
			node->set_name("Unknown_" + ChildInfo.ObjectName);
			node->set_transform(ChildInfo.godot_trans);
			root->add_child(node);
			curr_node = node;
		} break;
		case FbxNodeAttribute::eNull: {
			ChildInfo.AttributeType = "eNull";
			Spatial *node = memnew(Spatial);
			node->set_name("Null_" + ChildInfo.ObjectName);
			curr_node = node;
		} break;
		case FbxNodeAttribute::eMarker: {
			ChildInfo.AttributeType = "eMarker";
			Spatial *node = memnew(Spatial);
			node->set_name("Marker_" + ChildInfo.ObjectName);
			curr_node = node;
		} break;
		case FbxNodeAttribute::eSkeleton:
			ChildInfo.AttributeType = "eSkeleton";
			{
				skin = memnew(Skeleton);
				skin->set_name("skin_" + ChildInfo.ObjectName);
				AllSkin.insert(fbx_node, skin);
				// 设置所属谷歌
				ChildInfo.SkelectID = AllSkin.size();
				IsSkelete = true;
			}
			break;
		case FbxNodeAttribute::eMesh:
			ChildInfo.AttributeType = "eMesh";
			{
				MeshInstance *mesh = new MeshInstance();
				mesh->set_name("mesh_" + ChildInfo.ObjectName);
				curr_node = mesh;
				AllMesh.insert(fbx_node, mesh);
			}
			break;
		case FbxNodeAttribute::eNurbs: {
			ChildInfo.AttributeType = "eNurbs";
			Spatial *node = memnew(Spatial);
			node->set_name("Nurbs_" + ChildInfo.ObjectName);
			curr_node = node;
		} break;
		case FbxNodeAttribute::ePatch: {
			ChildInfo.AttributeType = "ePatch";
			Spatial *node = memnew(Spatial);
			node->set_name("Patch_" + ChildInfo.ObjectName);
			curr_node = node;
		} break;
		case FbxNodeAttribute::eCamera: {
			ChildInfo.AttributeType = "eCamera";
			Spatial *node = memnew(Spatial);
			node->set_name("Camera_" + ChildInfo.ObjectName);
			curr_node = node;
		} break;
		case FbxNodeAttribute::eCameraStereo: {
			ChildInfo.AttributeType = "eCameraStereo";
			Spatial *node = memnew(Spatial);
			node->set_name("CameraStereo_" + ChildInfo.ObjectName);
			curr_node = node;
		} break;
		case FbxNodeAttribute::eCameraSwitcher: {
			ChildInfo.AttributeType = "eCameraSwitcher";
			Spatial *node = memnew(Spatial);
			node->set_name("CameraSwitcher_" + ChildInfo.ObjectName);
			root->add_child(node);
		} break;
		case FbxNodeAttribute::eLight: {
			ChildInfo.AttributeType = "eLight";
			Spatial *node = memnew(Spatial);
			node->set_name("Light_" + ChildInfo.ObjectName);
			curr_node = node;
		} break;
		case FbxNodeAttribute::eOpticalReference: {
			ChildInfo.AttributeType = "eOpticalReference";
			Spatial *node = memnew(Spatial);
			node->set_name("OpticalReference_" + ChildInfo.ObjectName);
			curr_node = node;
		} break;
		case FbxNodeAttribute::eOpticalMarker: {
			ChildInfo.AttributeType = "eOpticalMarker";
			Spatial *node = memnew(Spatial);
			node->set_name("OpticalMarker_" + ChildInfo.ObjectName);
			curr_node = node;
		} break;
		case FbxNodeAttribute::eNurbsCurve: {
			ChildInfo.AttributeType = "eNurbsCurve";
			Spatial *node = memnew(Spatial);
			node->set_name("NurbsCurve_" + ChildInfo.ObjectName);
			curr_node = node;
		} break;
		case FbxNodeAttribute::eTrimNurbsSurface: {
			ChildInfo.AttributeType = "eTrimNurbsSurface";
			Spatial *node = memnew(Spatial);
			node->set_name("TrimNurbsSurface_" + ChildInfo.ObjectName);
			root->add_child(node);
			curr_node = node;
		} break;
		case FbxNodeAttribute::eBoundary: {
			ChildInfo.AttributeType = "eBoundary";
			Spatial *node = memnew(Spatial);
			node->set_name("Boundary_" + ChildInfo.ObjectName);
			curr_node = node;
		} break;
		case FbxNodeAttribute::eNurbsSurface: {
			ChildInfo.AttributeType = "eNurbsSurface";
			Spatial *node = memnew(Spatial);
			node->set_name("NurbsSurface_" + ChildInfo.ObjectName);
			curr_node = node;
		} break;
		case FbxNodeAttribute::eShape: {
			ChildInfo.AttributeType = "eShape";
			Spatial *node = memnew(Spatial);
			node->set_name("Shape_" + ChildInfo.ObjectName);
			curr_node = node;
		} break;
		case FbxNodeAttribute::eLODGroup: {
			ChildInfo.AttributeType = "eLODGroup";
			Spatial *node = memnew(Spatial);
			node->set_name("LODGroup_" + ChildInfo.ObjectName);
			curr_node = node;
		} break;
		case FbxNodeAttribute::eSubDiv: {
			ChildInfo.AttributeType = "eSubDiv";
			Spatial *node = memnew(Spatial);
			node->set_name("SubDiv_" + ChildInfo.ObjectName);
			curr_node = node;
		} break;
		case FbxNodeAttribute::eCachedEffect: {
			ChildInfo.AttributeType = "eCachedEffect";
			Spatial *node = memnew(Spatial);
			node->set_name("CachedEffect_" + ChildInfo.ObjectName);
			curr_node = node;
		} break;
		case FbxNodeAttribute::eLine: {
			ChildInfo.AttributeType = "eLine";
			Spatial *node = memnew(Spatial);
			node->set_name("Line_" + ChildInfo.ObjectName);
			curr_node = node;
		} break;
	}
	curr_node->set_transform(ChildInfo.godot_trans);
	root->add_child(curr_node);

	HierarchyInfo.push_back(ChildInfo);
	// 处理所有子节点
	int32_t NodeCount = fbx_node->GetChildCount();
	for (int32_t NodeIndex = 0; NodeIndex < NodeCount; ++NodeIndex) {
		if (IsSkelete) {
			LoadSkelect(skin, fbx_node->GetChild(NodeIndex), fbx_node);

		} else {
			ProcessNode(fbx_node->GetChild(NodeIndex), fbx_node, curr_node);
		}
	}
}
// 家在谷歌信息
void Fbx_file::LoadSkelect(Skeleton *skin, FbxNode *node, FbxNode *root_node) {
	FbxNodeInfo ChildInfo;
	ChildInfo.ObjectName = node->GetName();
	ChildInfo.UniqueId = node->GetUniqueID();
	ChildInfo.ParentName = root_node->GetName();
	ChildInfo.ParentUniqueId = root_node->GetUniqueID();
	ChildInfo.RotationPivot = node->RotationPivot.Get();
	ChildInfo.ScalePivot = node->ScalingPivot.Get();
	ChildInfo.transform = node->EvaluateLocalTransform();
	ChildInfo.SkelectID = AllSkin.size();
	ChildInfo.SourceNode = node;
	// 转换一下矩阵
	ChildInfo.godot_trans = Fbx_utils::to_transfo(ChildInfo.transform);

	int32_t NodeCount = node->GetChildCount();
	String name = node->GetName();
	skin->add_bone(name);
	int bone_id = skin->find_bone(name);
	int bone_parent_id = skin->find_bone(ChildInfo.ParentName);
	// 设定出事姿势
	skin->set_bone_rest(bone_id, ChildInfo.godot_trans);
	skin->set_bone_pose(bone_id, ChildInfo.godot_trans);
	// 设置骨骼的父节点
	skin->set_bone_parent(bone_id, bone_parent_id);

	HierarchyInfo.push_back(ChildInfo);
	for (int32_t NodeIndex = 0; NodeIndex < NodeCount; ++NodeIndex) {
		LoadSkelect(skin, node->GetChild(NodeIndex), node);
	}
}

// -----------------------------------------------------------------------------

void Fbx_file::get_mesh(Abs_mesh &mesh) {
	mesh._mesh_path = _path;
	_offset_verts.clear();
	// transform into intermediary structure
	for (int i = 0; i < _fbx_scene->GetNodeCount(); i++) {
		FbxNode *node = _fbx_scene->GetNode(i);

		// upgrade structure from node content
		FbxNodeAttribute *attr = node->GetNodeAttribute();
		if (attr != 0) {
			switch (attr->GetAttributeType()) {
				// TODO: extend NodeAttributes handling
				case FbxNodeAttribute::eSkeleton:
					break;
				case FbxNodeAttribute::eMesh: {
					_offset_verts[node] = mesh._vertices.size();
					FbxMesh *fbx_mesh = (FbxMesh *)attr;
					Ref<ArrayMesh> godot_mesh;
					godot_mesh.instance();
					FBX_LoadContext context;
					context.godot_mesh = godot_mesh;
					fill_mesh(this, fbx_mesh, node, mesh, context);
				} break;
				default:
					break;
			}
		}
		//... TODO: deal with transforms, properties ...
	}

	_size_mesh = mesh._vertices.size();

	for (unsigned i = 0; i < mesh._materials.size(); i++)
		mesh._materials[i].set_relative_paths(_path);
}

// -----------------------------------------------------------------------------

/// Fill skell with the FBx skeleton hierachy. Only links between parents and
/// nodes are stored.
static int fill_skeleton(Abs_skeleton &skel,
		std::map<FbxNode *, int> &ptr_to_idx,
		int bone_parent,
		FbxNode *node) {
	if (bone_parent < 0)
		skel._root = 0; // First node in srd::vector is root

	const FbxNodeAttribute *attr = node->GetNodeAttribute();
	assert(attr->GetAttributeType() == FbxNodeAttribute::eSkeleton ||
			attr->GetAttributeType() == FbxNodeAttribute::eNull);

	const FbxSkeleton *skel_attr = (const FbxSkeleton *)attr;
	String name(skel_attr->GetNameOnly().Buffer());

	//FbxAnimEvaluator::GetNodeGlobalTransformFast()
	//Transfo tr = Fbx_utils::to_transfo( Fbx_utils::geometry_transfo(node) );
	Transform tr = Fbx_utils::to_transfo(node->EvaluateGlobalTransform());
	Abs_bone bone = { 0.f, tr, name };
	skel._bones.push_back(bone);
	skel._parents.push_back(bone_parent);
	skel._sons.push_back(std::vector<int>());
	int bone_idx = skel._bones.size() - 1;
	ptr_to_idx[node] = bone_idx;

	unsigned nb_children = node->GetChildCount();
	std::vector<int> sons(nb_children);
	for (unsigned c = 0; c < nb_children; c++) {
		FbxNode *child = node->GetChild(c);
		// Recursive lookup
		int cidx = fill_skeleton(skel, ptr_to_idx, bone_idx, child);
		sons[c] = cidx;
	}
	skel._sons[bone_idx] = sons;
	return bone_idx;
}

// -----------------------------------------------------------------------------

typedef std::map<FbxNode *, int> Node_map;

void set_skel_frame(Abs_skeleton &skel,
		FbxNode *node,
		const std::map<FbxNode *, int> &ptr_to_idx,
		const FbxMatrix &mat) {
	const Transform tr = Fbx_utils::to_transfo(mat);
	Node_map::const_iterator it = ptr_to_idx.find(node);
	if (it != ptr_to_idx.end())
		skel._bones[it->second]._frame = tr;
	else {
		std::cerr << "WARNING FBX: unkonwn node reference in bones list '";
		std::cerr << node->GetName() << "'" << std::endl;
	}
}

// -----------------------------------------------------------------------------

/// Compute the frame of every bone from the bind pose
/// @return if we succeded to compute every frames
void compute_bones_bind_frame(Abs_skeleton &skel,
		FbxPose *pose,
		const std::map<FbxNode *, int> &ptr_to_idx) {
	for (int i = 0; i < pose->GetCount(); i++) {
		FbxNode *node = pose->GetNode(i);
		const FbxMatrix mat = pose->GetMatrix(i);
		set_skel_frame(skel, node, ptr_to_idx, mat);
	}
}

// -----------------------------------------------------------------------------

/// @return the bind frame
FbxMatrix compute_cluster_bind_frame(FbxGeometry *geom,
		FbxCluster *cluster) {
	FbxCluster::ELinkMode clus_mode = cluster->GetLinkMode();

	if (clus_mode == FbxCluster::eAdditive && cluster->GetAssociateModel()) {
		std::cerr << "WARNING FBX: cluster is eADDITIVE we don't handle";
		std::cerr << "this type of skinning" << std::endl;
	}

	FbxAMatrix clus_transfo;
	// TransformMatrix refers to the global initial transform of the geometry
	// node that contains the link node. (i.e global transfo of 'geom')
	cluster->GetTransformMatrix(clus_transfo);

	FbxAMatrix geom_transfo = Fbx_utils::geometry_transfo(geom->GetNode());

	FbxAMatrix clus_link_transfo;
	// TransformLink refers to global initial transform of the link node (
	// (i.e global transfo of the bone associated to this cluster).
	cluster->GetTransformLinkMatrix(clus_link_transfo);

	//return lClusterRelativeCurrentPositionInverse * lClusterRelativeInitPosition;
	return (clus_transfo * geom_transfo).Inverse() * clus_link_transfo;
}

// -----------------------------------------------------------------------------

void compute_bones_bind_frame(Abs_skeleton &skel,
		std::map<FbxNode *, FbxNode *> &done,
		FbxSkin *skin,
		FbxGeometry *geom,
		const std::map<FbxNode *, int> &ptr_to_idx) {
	done.clear();
	for (int i = 0; i < skin->GetClusterCount(); i++) {
		FbxCluster *cluster = skin->GetCluster(i);
		FbxNode *cl_node = cluster->GetLink();

		if (cluster->GetLink() == 0)
			continue;

		done[cl_node] = cl_node;

		FbxMatrix mat = compute_cluster_bind_frame(geom, cluster);
		set_skel_frame(skel, cl_node, ptr_to_idx, FbxMatrix(mat));
	}
}

// -----------------------------------------------------------------------------

void fill_bones_weights(Abs_skeleton &skel,
		FbxSkin *skin,
		const std::map<FbxNode *, int> &ptr_to_idx,
		int off,
		int size_mesh) {
	skel._weights.resize(size_mesh);
	for (int i = 0; i < skin->GetClusterCount(); i++) {
		FbxCluster *cluster = skin->GetCluster(i);
		FbxNode *cl_node = cluster->GetLink();

		if (cl_node == 0)
			continue;

		const int bone_id = ptr_to_idx.find(cl_node)->second;

		const int size_cluster = cluster->GetControlPointIndicesCount();
		for (int c = 0; c < size_cluster; c++) {
			const int fbx_idx = cluster->GetControlPointIndices()[c];
			const float fbx_weight = (float)cluster->GetControlPointWeights()[c];

			std::pair<int, float> p = std::make_pair(bone_id, fbx_weight);

			skel._weights[fbx_idx + off].push_back(p);
		}
	}
}

// -----------------------------------------------------------------------------

/// Extract bind pose using FbxPose but For some files this won't work
void fill_bind_pose(FbxScene *scene,
		Abs_skeleton &skel,
		std::map<FbxNode *, int> ptr_to_idx) {
	int nb_poses = scene->GetPoseCount();
	int nb_bind_poses = 0;
	for (int i = 0; i < nb_poses; i++) {
		FbxPose *pose = scene->GetPose(i);
		if (pose->IsBindPose()) {
			compute_bones_bind_frame(skel, pose, ptr_to_idx);
			nb_bind_poses++;
		}
	}

	if (nb_bind_poses > 1) {
		std::cerr << "WARNING FBX: there is more than one bind pose!";
		std::cerr << std::endl;
	} else if (nb_bind_poses == 0) {
		std::cerr << "WARNING FBX: there is no bind pose!\n";
		std::cerr << "We try to compute it with LinkMatrix";
	}
}

// -----------------------------------------------------------------------------

/// compute bind pose for nodes without clusters
/// @param done: list of nodes pointer which bind pose are correct and
/// associated to a cluster
void fill_bind_pose_cluster_less_nodes(
		FbxScene *scene,
		Abs_skeleton &skel,
		const std::map<FbxNode *, FbxNode *> &done,
		const std::map<FbxNode *, int> &ptr_to_idx) {
	for (int i = 0; i < scene->GetNodeCount(); i++) {
		FbxNode *node = scene->GetNode(i);

		FbxNodeAttribute *attr = node->GetNodeAttribute();
		// If not in the map 'done' means the bind pose is not computed
		if (attr != 0 &&
				attr->GetAttributeType() == FbxNodeAttribute::eSkeleton &&
				!Std_utils::exists(done, node)) {
			FbxNode *parent = node->GetParent();

			// Retreive bind pose of the parent bone
			const int *pid = 0;
			Transform bind_pose_prt;
			if (Std_utils::find(ptr_to_idx, parent, pid))
				bind_pose_prt = skel._bones[*pid]._frame;

			// Retreive local transformation of the bone
			int id = Std_utils::find(ptr_to_idx, node);
			Transform lc = Fbx_utils::to_transfo(node->EvaluateLocalTransform());
			Transform tr = bind_pose_prt * lc;
			skel._bones[id]._frame = tr;
		}
	}
}

// -----------------------------------------------------------------------------

/// Build the map that associates a FbxNode pointer to its index in the
/// structure Loader::Abs_skeleton._bones[index]
void get_nodes_ptr_to_bone_id(const FbxScene *scene,
		std::map<FbxNode *, int> &ptr_to_idx) {
	ptr_to_idx.clear();
	FbxNode *root = scene->GetRootNode();
	root = Fbx_utils::find_root(root, FbxNodeAttribute::eSkeleton);
	if (root == 0)
		return;
	Abs_skeleton skel;
	fill_skeleton(skel, ptr_to_idx, -1, root);
}

// -----------------------------------------------------------------------------

/// Build the vector that associates an index in the
/// structure Loader::Abs_skeleton._bones[index] to a FbxNode pointer
void get_bone_id_to_nodes_ptr(const FbxScene *scene,
		std::vector<FbxNode *> &idx_to_ptr) {
	idx_to_ptr.clear();
	std::map<FbxNode *, int> ptr_to_idx;
	get_nodes_ptr_to_bone_id(scene, ptr_to_idx);

	idx_to_ptr.resize(ptr_to_idx.size());
	std::map<FbxNode *, int>::const_iterator it;
	for (it = ptr_to_idx.begin(); it != ptr_to_idx.end(); ++it)
		idx_to_ptr[it->second] = it->first;
}

// -----------------------------------------------------------------------------

void Fbx_file::get_skeleton(Abs_skeleton &skel) const {
	FbxNode *root = _fbx_scene->GetRootNode();
	root = Fbx_utils::find_root(root, FbxNodeAttribute::eSkeleton);

	if (root == 0)
		return;
	//Fbx_utils::print_hierarchy(_fbx_scene);

	std::map<FbxNode *, int> ptr_to_idx;
	// Building the skeleton hierarchy
	fill_skeleton(skel, ptr_to_idx, -1, root);

#if 0
    // Extract bind pose using FbxPose but For some files this won't work
    fill_bind_pose(_fbx_scene, skel, ptr_to_idx);
#else

	// List of bone/node whose bind pose are computed
	std::map<FbxNode *, FbxNode *> done;
	for (int i = 0; i < _fbx_scene->GetGeometryCount(); i++) {
		FbxGeometry *geom = _fbx_scene->GetGeometry(i);
		const FbxNode *node = geom->GetNode();
		if (geom != 0 && geom->GetAttributeType() == FbxNodeAttribute::eMesh) {
			if (geom->GetDeformerCount(FbxDeformer::eSkin) > 1) {
				std::cerr << "WARNING FBX: there is more than one skin deformer";
				std::cerr << "associated to the geometry: " << geom->GetName();
				std::cerr << std::endl;
			}

			FbxDeformer *dfm = geom->GetDeformer(0, FbxDeformer::eSkin);

			if (dfm == 0) {
				std::cerr << "WARNING FBX: there is no deformer associated to";
				std::cerr << "the geometry: " << geom->GetName() << std::endl;
				continue;
			}

			// Extract bind pose using clusters
			compute_bones_bind_frame(skel, done, (FbxSkin *)dfm, geom, ptr_to_idx);

			const int offset = Std_utils::find(_offset_verts, node);
			// Extract bones weights
			fill_bones_weights(skel, (FbxSkin *)dfm, ptr_to_idx, offset, _size_mesh);
		}
	}

	// Some bind pose can't be calculated with clusters because nodes does not
	// influence the mesh. This little trick tries to compute them
	fill_bind_pose_cluster_less_nodes(_fbx_scene, skel, done, ptr_to_idx);

#endif
	compute_bone_lengths(skel);
}

// -----------------------------------------------------------------------------

/// Sample the animation 'anim_stack' and store it in 'abs_anim'
/// @param abs_anim: animation evaluator to fill
/// @param anim_stack: animation used to fill 'abs_anim'
/// @param scene: FBX scene needed to evaluate the animation
/// @param skel: we need the bind pose matrices to convert animation matrices
/// from Global to local space of the bones
bool fill_anim(Sampled_anim_eval *abs_anim,
		FbxAnimStack *anim_stack,
		FbxScene *scene,
		const std::vector<FbxNode *> &idx_to_ptr,
		const Abs_skeleton &skel) {
	if (anim_stack == 0)
		return false;

	// we assume that the first animation layer connected to the animation
	// stack is the base layer (this is the assumption made in the FBXSDK)
	//FbxAnimLayer* mCurrentAnimLayer = anim_stack->GetMember(FBX_TYPE(FbxAnimLayer), 0);

	//scene->GetEvaluator()->SetContext(anim_stack);
	scene->SetCurrentAnimationStack(anim_stack);

	FbxTime frame_inter, start, stop;
	FbxTime::EMode time_mode = scene->GetGlobalSettings().GetTimeMode();

	const double fps = FbxTime::GetFrameRate(time_mode) * 8; /* HACK: increase fps to sample more anim frames -----------------------------*/ // ARMA = 4

	frame_inter.SetSecondDouble(1. / fps);

	FbxTakeInfo *take_info = scene->GetTakeInfo(anim_stack->GetName());
	if (take_info) {
		start = take_info->mLocalTimeSpan.GetStart();
		stop = take_info->mLocalTimeSpan.GetStop();
	} else {
		// Take the time line value
		FbxTimeSpan lTimeLineTimeSpan;
		scene->GetGlobalSettings().GetTimelineDefaultTimeSpan(lTimeLineTimeSpan);

		start = lTimeLineTimeSpan.GetStart();
		stop = lTimeLineTimeSpan.GetStop();
	}

	if (fps <= 0.) {
		std::cerr << "FBX ERROR: frame rate is not set properly" << std::endl;
		return false;
	}

	// Sampling matrices for every frames
	int nb_frame_approx = (int)((stop - start).GetSecondDouble() * fps) + 1;
	abs_anim->_lcl_frames.reserve(nb_frame_approx);
	abs_anim->_frame_rate = (float)fps;
	for (FbxTime t = start; t < stop; t += frame_inter) {
		int nb_bones = idx_to_ptr.size();
		std::vector<Transform> pose_t(nb_bones);
		for (int i = 0; i < nb_bones; i++) {
			FbxNode *node = idx_to_ptr[i];
			Transform tr = Fbx_utils::to_transfo(node->EvaluateGlobalTransform(t));

			Transform joint_inv = skel._bones[i]._frame.inverse();

			pose_t[i] = joint_inv * tr;
		}
		abs_anim->_lcl_frames.push_back(pose_t);
	}

	return true;
}

// -----------------------------------------------------------------------------

void Fbx_file::get_animations(std::vector<Base_anim_eval *> &anims) const {
	Abs_skeleton skel;
	get_skeleton(skel);

	std::vector<FbxNode *> idx_to_ptr;
	get_bone_id_to_nodes_ptr(_fbx_scene, idx_to_ptr);

	FbxScene *scene = _fbx_scene;

	/*
    FbxNode* root = scene->GetRootNode();
    const float framerate = static_cast<float>(FbxTime::GetFrameRate(scene->GetGlobalSettings().GetTimeMode()));
    root->ResetPivotSetAndConvertAnimation( framerate, false, false );
    */

	int nb_stacks = _fbx_scene->GetSrcObjectCount<FbxAnimStack>();
	for (int i = 0; i < nb_stacks; i++) {
		// Extract the ith animation
		FbxObject *obj = scene->GetSrcObject<FbxAnimStack>(i);
		FbxAnimStack *stack = FbxCast<FbxAnimStack>(obj);
		std::string str(stack->GetName());
		Sampled_anim_eval *abs_anim = new Sampled_anim_eval(str);
		if (fill_anim(abs_anim, stack, scene, idx_to_ptr, skel))
			anims.push_back(abs_anim);
	}
}

// -----------------------------------------------------------------------------

void Fbx_file::set_mesh(const Abs_mesh &mesh) {
}

// -----------------------------------------------------------------------------

#if 0
bool SaveScene(FbxDocument* fbx_scene, const char* pFilename, int pFileFormat, bool pEmbedMedia)
{
    int lMajor, lMinor, lRevision;
    bool lStatus = true;

    // Create an exporter.
    FbxExporter* lExporter = FbxExporter::Create(g_FBXSdkManager, "");

    if( pFileFormat < 0 || pFileFormat >= g_FBXSdkManager->GetIOPluginRegistry()->GetWriterFormatCount() )
    {
        // Write in fall back format if pEmbedMedia is true
        pFileFormat = g_FBXSdkManager->GetIOPluginRegistry()->GetNativeWriterFormat();

        if (!pEmbedMedia)
        {
            //Try to export in ASCII if possible
            int lFormatIndex, lFormatCount = g_FBXSdkManager->GetIOPluginRegistry()->GetWriterFormatCount();

            for (lFormatIndex=0; lFormatIndex<lFormatCount; lFormatIndex++)
            {
                if (g_FBXSdkManager->GetIOPluginRegistry()->WriterIsFBX(lFormatIndex))
                {
                    FbxString lDesc =g_FBXSdkManager->GetIOPluginRegistry()->GetWriterFormatDescription(lFormatIndex);
                    char *lASCII = "ascii";
                    if (lDesc.Find(lASCII)>=0)
                    {
                        pFileFormat = lFormatIndex;
                        break;
                    }
                }
            }
        }
    }

    // Set the export states. By default, the export states are always set to
    // true except for the option eEXPORT_TEXTURE_AS_EMBEDDED. The code below
    // shows how to change these states.

    IOS_REF.SetBoolProp(EXP_FBX_MATERIAL,        true);
    IOS_REF.SetBoolProp(EXP_FBX_TEXTURE,         true);
    IOS_REF.SetBoolProp(EXP_FBX_EMBEDDED,        pEmbedMedia);
    IOS_REF.SetBoolProp(EXP_FBX_SHAPE,           true);
    IOS_REF.SetBoolProp(EXP_FBX_GOBO,            true);
    IOS_REF.SetBoolProp(EXP_FBX_ANIMATION,       true);
    IOS_REF.SetBoolProp(EXP_FBX_GLOBAL_SETTINGS, true);

    // Initialize the exporter by providing a filename.
    if(lExporter->Initialize(pFilename, pFileFormat, g_FBXSdkManager->GetIOSettings()) == false)
    {
        printf("Call to FbxExporter::Initialize() failed.\n");
        printf("Error returned: %s\n\n", lExporter->GetLastErrorString());
        return false;
    }

    FbxSdkManager::GetFileFormatVersion(lMajor, lMinor, lRevision);
    printf("FBX version number for this version of the FBX SDK is %d.%d.%d\n\n", lMajor, lMinor, lRevision);

    // Export the scene.
    lStatus = lExporter->Export(fbx_scene);

    // Destroy the exporter.
    lExporter->Destroy();
    return lStatus;
}
#endif

// -----------------------------------------------------------------------------

void Fbx_file::free_mem() {
	if (_fbx_scene != 0) {
		_fbx_scene->Destroy();
		_fbx_scene = 0;
	}
}

// -----------------------------------------------------------------------------

} // namespace Loader
