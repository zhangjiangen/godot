#pragma once

#include "SkinnedData.h"
#include "core/color.h"
#include "core/math/vector2.h"
#include "scene/3d/mesh_instance.h"
#include "scene/3d/skeleton.h"
#include "scene/3d/spatial.h"
#include "scene/animation/animation_player.h"
#include "scene/resources/surface_tool.h"
#include <fbxsdk.h>
#include <map>

struct BoneIndexAndWeight {
	int mBoneIndices;
	float mBoneWeight;

	bool operator<(const BoneIndexAndWeight &rhs) const {
		return (mBoneWeight > rhs.mBoneWeight);
	}
};

struct CtrlPoint {
	Vector3 mPosition;
	std::vector<BoneIndexAndWeight> mBoneInfo;
	String mBoneName;

	CtrlPoint() {
		mBoneInfo.reserve(4);
	}

	void SortBlendingInfoByWeight() {
		std::sort(mBoneInfo.begin(), mBoneInfo.end());
	}
};

struct Vertex {
	Vector3 Pos;
	Color color;
	Vector3 Normal;
	Vector2 TexC;
	Vector2 TexC2;
	Vector2 TexC3;
	Vector2 TexC4;
	int vertexID;

	bool
	operator==(const Vertex &other) const {
		if (Pos.x != other.Pos.x || Pos.y != other.Pos.y || Pos.z != other.Pos.z)
			return false;

		if (Normal.x != other.Normal.x || Normal.y != other.Normal.y || Normal.z != other.Normal.z)
			return false;

		if (TexC.x != other.TexC.x || TexC.y != other.TexC.y)
			return false;

		return true;
	}
};
struct VertexHashFunc {
	std::size_t operator()(const Vertex &c) const {
		return std::hash<float>()(c.Pos.x) + std::hash<float>()(c.Pos.y) + std::hash<float>()(c.Pos.z) + std::hash<float>()(c.Normal.x) + std::hash<float>()(c.Normal.y) + std::hash<float>()(c.Normal.z) + std::hash<float>()(c.TexC.x) + std::hash<float>()(c.TexC.y);
	}
};

struct CharacterVertex : Vertex {
	Vector3 BoneWeights;
	uint8_t BoneIndices[4];

	uint16_t MaterialIndex;
};
struct FbxMaterial {
	// Unique material name for lookup.
	String Name;
	bool IsTwoSide = false;
	// Index into constant buffer corresponding to this material.
	int MatCBIndex = -1;

	// Index into SRV heap for diffuse texture.
	int DiffuseSrvHeapIndex = -1;

	// Index into SRV heap for normal texture.
	int NormalSrvHeapIndex = -1;

	// Dirty flag indicating the material has changed and we need to update the constant buffer.
	// Because we have a material constant buffer for each FrameResource, we have to apply the
	// update to each FrameResource.  Thus, when we modify a material we should set
	// NumFramesDirty = gNumFrameResources so that each frame resource gets the update.
	int NumFramesDirty = 3;

	// Material constant buffer data used for shading.
	Color Ambient = { 0.0f, 0.0f, 0.0f, 1 };
	Color DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	Color FresnelR0 = { 0.01f, 0.01f, 0.01f, 1 };
	Color Specular = { 0.01f, 0.01f, 0.01f, 1 };
	Color Emissive = { 0.01f, 0.01f, 0.01f, 1 };

	float Roughness = .25f;
	Transform MatTransform;
};
class FbxLoader {
public:
	struct SkinnedLoadData {
		std::vector<String> mBoneName;
		std::vector<int> mBoneHierarchy;
		std::vector<Transform> mBoneOffsets;
		fbxsdk::FbxNode *RootNode;
	};
	struct MeshLoadData {
		String MeshName;
		MeshInstance *MeshIns = nullptr;
		Spatial *pSekeleton = nullptr;
		std::vector<FbxMaterial> material;
		Ref<SurfaceTool> vertex;
		Vector<Ref<SurfaceTool>> morphs;
		Transform geometryTransform;

		Ref<SpatialMaterial> GetMaterial(int index) {
			Ref<SpatialMaterial> spatial_material;
			spatial_material.instance();
			FbxMaterial &load_mat = material[index];
			// 设置材质名称
			spatial_material->set_name(load_mat.Name);
			return spatial_material;
		}
		Ref<ShaderMaterial> GetShaderMaterial(String load_path, int index);
		Ref<Skin> skin;
	};
	struct MeshBoneWeightData {
		Map<unsigned int, CtrlPoint> mControlPoints;
	};
	struct SekeletonAnimationData {
		Map<String, AnimationClip *> mAnimations;
		// 谷歌的多边形变换信息
		fbxsdk::FbxAMatrix geometryTransform;
		void GetAnimation(AnimationPlayer *anim_play, Spatial *root_node, Skeleton *ske);
	};

public:
	FbxLoader();
	~FbxLoader();

	// Animation
	Spatial *LoadFBX(String fileName);
	void ProcessSkeletonHierarchy(fbxsdk::FbxNode *root, Spatial *parent_node);
	void ProcessMeshAndAnimation(fbxsdk::FbxScene *pFbxScene, fbxsdk::FbxNode *root, Spatial *parent_node);

	void GetSkeletonHierarchy(
			fbxsdk::FbxNode *pNode, fbxsdk::FbxNode *parentNode,
			SkinnedLoadData *outSkinnedData,
			int curIndex, int parentIndex);

	void GetAnimation(
			fbxsdk::FbxScene *pFbxScene,
			fbxsdk::FbxNode *pFbxChildNode, MeshInstance *meshInstance,
			const String &ClipName, bool isGetOnlyAnim);

	void GetVerticesAndIndice(
			fbxsdk::FbxMesh *pMesh,
			std::vector<CharacterVertex> &outVertexVector,
			std::vector<uint32_t> &outIndexVector,
			SkinnedData *outSkinnedData);
	void GetVerticesAndIndice(
			fbxsdk::FbxMesh *pMesh,
			std::vector<Vertex> &outVertexVector,
			std::vector<uint32_t> &outIndexVector);
	void GetVerticesAndIndice(fbxsdk::FbxNode *pNode, fbxsdk::FbxMesh *pMesh, Ref<SurfaceTool> &mesh);

	void GetMaterials(fbxsdk::FbxNode *pNode, std::vector<FbxMaterial> &outMaterial);

	void GetMaterialAttribute(fbxsdk::FbxSurfaceMaterial *pMaterial, FbxMaterial &outMaterial);

	void GetMaterialTexture(fbxsdk::FbxSurfaceMaterial *pMaterial, FbxMaterial &Mat);

	fbxsdk::FbxAMatrix GetGeometryTransformation(fbxsdk::FbxNode *pNode);

	void clear();

public:
	SekeletonAnimationData *GetSkinnedAnimationData(Spatial *node) {
		if (SekeletonAnimation.has(node)) {
			return SekeletonAnimation.find(node)->value();
		}
		return nullptr;
	}

	// 骨架的动画信息
	Map<Spatial *, SekeletonAnimationData *> SekeletonAnimation;

public:
	//std::vector<String> mBoneName;

	// skinnedData Output
	//std::vector<int> mBoneHierarchy;
	//std::vector<Transform> mBoneOffsets;
	// 通过骨骼获取到骨架家在数据
	SkinnedLoadData *GetSkinnedLoadData(fbxsdk::FbxNode *fbx_bone_node) {
		fbxsdk::FbxNode *node = fbx_bone_node;
		while (node) {
			if (TotalSkinnedLoadDataMap.has(node)) {
				return TotalSkinnedLoadDataMap.find(node)->value();
			}
			node = node->GetParent();
		}
		return nullptr;
	}
	Transform FbxMatrixToTransform(fbxsdk::FbxAMatrix mat) {
		fbxsdk::FbxVector4 T = mat.GetT();
		fbxsdk::FbxQuaternion Q = mat.GetQ();
		fbxsdk::FbxVector4 S = mat.GetS();
		Transform ret;
		ret.origin = Vector3(T[0], T[1], T[2]) * 0.01f;
		ret.basis.set_quat_scale(Quat(Q[0], Q[1], Q[2], Q[3]), Vector3(S[0], S[1], S[2]));
		return ret;
	}
	// 获取骨架节点
	Spatial *GetSkeleton(fbxsdk::FbxNode *fbx_bone_node) {
		fbxsdk::FbxNode *node = fbx_bone_node;
		while (node) {
			if (TotalNodeMap.has(node)) {
				return TotalNodeMap.find(node)->value();
			}
			node = node->GetParent();
		}
		return nullptr;
	}
	// 获取场景节点信息
	Spatial *GetSceneNode(fbxsdk::FbxNode *fbx_node) {
		if (TotalNodeMap.has(fbx_node)) {
			return TotalNodeMap.find(fbx_node)->value();
		}
		return nullptr;
	}
	// 获取模型的谷歌权重信息
	MeshBoneWeightData *GetMeshBoneWeightData(fbxsdk::FbxNode *fbx_bone_node) {
		if (MeshBoneWeight.has(fbx_bone_node)) {
			return MeshBoneWeight.find(fbx_bone_node)->value();
		}
		return nullptr;
	}
	// 获取模型家在数据
	MeshLoadData *GetMeshLoadData(fbxsdk::FbxNode *fbx_bone_node) {
		if (TotalMeshLoadDataMap.has(fbx_bone_node)) {
			return TotalMeshLoadDataMap.find(fbx_bone_node)->value();
		}
		return nullptr;
	}
	Spatial *RootNode;
	Map<fbxsdk::FbxNode *, SkinnedLoadData *> TotalSkinnedLoadDataMap;
	Map<fbxsdk::FbxNode *, MeshLoadData *> TotalMeshLoadDataMap;
	// FBX节点到场景节点场景节点
	Map<fbxsdk::FbxNode *, Spatial *> TotalNodeMap;
	// 模型的骨骼权重信息
	Map<fbxsdk::FbxNode *, MeshBoneWeightData *> MeshBoneWeight;
	Map<fbxsdk::FbxNode *, fbxsdk::FbxNode *> BoneBindMesh;
};