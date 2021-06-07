#include "FbxLoader.h"
#include "core/print_string.h"
#include <assert.h>
#include <fstream>
#include <vector>

using namespace fbxsdk;

FbxLoader::FbxLoader() {
}

FbxLoader::~FbxLoader() {
}

FbxManager *gFbxManager = nullptr;

static void importVertexColors_byControlPoint(FbxMesh *fbx_mesh,
		FbxGeometryElementVertexColor *elt_color,
		Map<int, Color> &color) {
	FbxColor n;
	if (elt_color->GetReferenceMode() == FbxGeometryElement::eDirect) {
		for (int i = 0; i < fbx_mesh->GetControlPointsCount(); ++i) {
			n = elt_color->GetDirectArray().GetAt(i);
			color[i] = Color(n[0], n[1], n[2], n[3]);
		}
	} else {
		for (int i = 0; i < fbx_mesh->GetControlPointsCount(); ++i) {
			n = elt_color->GetDirectArray().GetAt(elt_color->GetIndexArray().GetAt(i));
			color[i] = Color(n[0], n[1], n[2], n[3]);
		}
	}
}

// -----------------------------------------------------------------------------

static void importVertexColors_byPolygonVertex(fbxsdk::FbxMesh *fbx_mesh,
		FbxGeometryElementVertexColor *elt_color,
		Map<int, Color> &color) {
	FbxColor n;
	// map of indices of normals, in order to quickly know if already seen
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
			n = elt_color->GetDirectArray().GetAt(lNormalIndex);
			color[lNormalIndex] = Color(n[0], n[1], n[2], n[3]);

			lIndexByPolygonVertex++;
		}
	}
}

void FbxLoader::GetSkeletonHierarchy(
		fbxsdk::FbxNode *pNode,
		SkinnedLoadData *outSkinnedData,
		int curIndex, int parentIndex) {
	Skeleton *ske = reinterpret_cast<Skeleton *>(GetSkeleton(pNode));
	Transform trans = FbxMatrixToTransform(pNode->EvaluateLocalTransform());
	ske->add_bone(pNode->GetName());
	ske->set_bone_parent(curIndex, parentIndex);
	ske->set_bone_pose(curIndex, trans);
	ske->set_bone_rest(curIndex, trans);

	outSkinnedData->mBoneHierarchy.push_back(parentIndex);
	outSkinnedData->mSkinnedData.SetBoneName(pNode->GetName());

	for (int i = 0; i < pNode->GetChildCount(); ++i) {
		GetSkeletonHierarchy(pNode->GetChild(i), outSkinnedData, outSkinnedData->mBoneHierarchy.size(), curIndex);
	}
}
void FbxLoader::ProcessSkeletonHierarchy(fbxsdk::FbxNode *pFbxRootNode, Spatial *parent_node) {
	for (int i = 0; i < pFbxRootNode->GetChildCount(); i++) {
		fbxsdk::FbxNode *pFbxChildNode = pFbxRootNode->GetChild(i);
		fbxsdk::FbxMesh *pMesh = (fbxsdk::FbxMesh *)pFbxChildNode->GetNodeAttribute();
		fbxsdk::FbxNodeAttribute::EType AttributeType = pMesh->GetAttributeType();

		if (!pMesh || !AttributeType) {
			continue;
		}
		Spatial *child_node = nullptr;
		switch (AttributeType) {
			case fbxsdk::FbxNodeAttribute::eSkeleton: {
				// 保存Skin加载数据
				SkinnedLoadData *data = memnew(SkinnedLoadData);
				TotalSkinnedLoadDataMap.insert(pFbxChildNode, data);
				child_node = memnew(Skeleton);
				TotalNodeMap.insert(pFbxChildNode, child_node);
				GetSkeletonHierarchy(pFbxChildNode, data, 0, -1);
				child_node->set_name(pFbxChildNode->GetName());
				parent_node->add_child(child_node);
				child_node->set_owner(RootNode);

			} break;
			case fbxsdk::FbxNodeAttribute::eMesh:
				break;
			default: {
				child_node = memnew(Spatial);
				Transform trans = FbxMatrixToTransform(pFbxChildNode->EvaluateLocalTransform());
				TotalNodeMap.insert(pFbxChildNode, child_node);
				child_node->set_transform(trans);
				parent_node->add_child(child_node);
				child_node->set_name(pFbxChildNode->GetName());
				child_node->set_owner(RootNode);
			}
		}
	}
}

void FbxLoader::ProcessMeshAndAnimation(fbxsdk::FbxScene *pFbxScene, fbxsdk::FbxNode *root, Spatial *parent_node) {
	int ii = root->GetChildCount();
	for (int i = 0; i < root->GetChildCount(); i++) {
		fbxsdk::FbxNode *pFbxChildNode = root->GetChild(i);
		fbxsdk::FbxMesh *pMesh = (fbxsdk::FbxMesh *)pFbxChildNode->GetNodeAttribute();
		fbxsdk::FbxNodeAttribute::EType AttributeType = pMesh->GetAttributeType();
		if (!pMesh || !AttributeType) {
			continue;
		}

		if (AttributeType == fbxsdk::FbxNodeAttribute::eMesh) {
			GetControlPoints(pFbxChildNode);
			MeshInstance *MeshIns = memnew(MeshInstance);
			Ref<SurfaceTool> st;
			// To access the bone index directly
			mBoneOffsets.resize(mBoneHierarchy.size());
			String clipName = "RootAnim";
			MeshLoadData *meshdata = memnew(MeshLoadData);
			meshdata->MeshIns = MeshIns;
			meshdata->MeshName = pFbxChildNode->GetName();
			TotalMeshLoadDataMap[pFbxChildNode] = meshdata;
			// Get Animation Clip
			GetAnimation(pFbxScene, pFbxChildNode, MeshIns, clipName, false);
			/*std::string outAnimationName;
				GetAnimation(pFbxScene, pFbxChildNode, outAnimationName, clipName);
				outSkinnedData.SetAnimationName(clipName);*/
			//outSkinnedData.SetAnimationName(outAnimationName);

			// Get Vertices and indices info
			GetVerticesAndIndice(pFbxChildNode, pMesh, meshdata->vertex);

			GetMaterials(pFbxChildNode, meshdata->material);

			break;
		} else if (AttributeType != fbxsdk::FbxNodeAttribute::eSkeleton) {
			// 处理所有子节点动画
			ProcessMeshAndAnimation(pFbxScene, pFbxChildNode, GetSceneNode(pFbxChildNode));
		}
	}
}

void FbxLoader::GetVerticesAndIndice(FbxNode *pNode,
		fbxsdk::FbxMesh *pMesh, Ref<SurfaceTool> &sf) {
	// 找到对应的骨骼权重信息
	MeshBoneWeightData *bone_weight_data = GetMeshBoneWeightData(pNode);
	uint32_t tCount = pMesh->GetPolygonCount(); // Triangle
	Map<int, Vertex> VeetexData;

	Map<int, Color> color;
	// 获取顶点颜色信息
	bool has_color = false;
	if (pMesh->GetElementVertexColorCount()) {
		has_color = true;
		FbxGeometryElementVertexColor *fbx_color = pMesh->GetElementVertexColor();
		FbxGeometryElement::EMappingMode type1 = fbx_color->GetMappingMode();
		if (type1 == FbxGeometryElement::eByControlPoint) {
			importVertexColors_byControlPoint(pMesh, fbx_color, color);
		} else if (type1 == FbxGeometryElement::eByPolygonVertex) {
			importVertexColors_byPolygonVertex(pMesh, fbx_color, color);
		}
	}
	Vector<int> IndexData;
	sf->begin();
	int uv_count = 0;
	for (uint32_t i = 0; i < tCount; ++i) {
		// For indexing by bone
		String CurrBoneName = mControlPoints[pMesh->GetPolygonVertex(i, 1)]->mBoneName;

		// Vertex and Index info
		for (int j = 0; j < 3; ++j) {
			int controlPointIndex = pMesh->GetPolygonVertex(i, j);
			// 保存索引信息
			IndexData.push_back(controlPointIndex);
			CtrlPoint *CurrCtrlPoint = mControlPoints[controlPointIndex];

			if (!VeetexData.has(controlPointIndex)) {
				// Normal
				FbxVector4 pNormal;
				pMesh->GetPolygonVertexNormal(i, j, pNormal);
				// UV
				float *lUVs = NULL;
				FbxStringList lUVNames;
				pMesh->GetUVSetNames(lUVNames);
				const char *lUVName = NULL;
				if (lUVNames.GetCount()) {
					lUVName = lUVNames[0];
				}

				FbxVector2 pUVs;
				bool bUnMappedUV;
				if (!pMesh->GetPolygonVertexUV(i, j, lUVName, pUVs, bUnMappedUV)) {
					print_verbose("UV not found");
				}

				Vertex Temp;
				if (lUVNames.GetCount() > uv_count) {
					uv_count = lUVNames.GetCount();
				}
				// Normal
				sf->add_normal(Vector3(pNormal.mData[0], pNormal.mData[1], pNormal.mData[2]));
				if (lUVNames.GetCount() > 0) {
					pMesh->GetPolygonVertexUV(i, j, lUVNames[0], pUVs, bUnMappedUV);
					Temp.TexC = (Vector2(pUVs[0], 1.0f - pUVs[1]));
				}
				if (lUVNames.GetCount() > 1) {
					pMesh->GetPolygonVertexUV(i, j, lUVNames[1], pUVs, bUnMappedUV);
					Temp.TexC2 = (Vector2(pUVs[0], 1.0f - pUVs[1]));
				}
				if (lUVNames.GetCount() > 2) {
					pMesh->GetPolygonVertexUV(i, j, lUVNames[2], pUVs, bUnMappedUV);
					Temp.TexC3 = (Vector2(pUVs[0], 1.0f - pUVs[1]));
				}
				if (lUVNames.GetCount() > 3) {
					pMesh->GetPolygonVertexUV(i, j, lUVNames[3], pUVs, bUnMappedUV);
					Temp.TexC4 = (Vector2(pUVs[0], 1.0f - pUVs[1]));
				}

				// UV
				Temp.TexC.x = static_cast<float>(pUVs.mData[0]);
				Temp.TexC.y = static_cast<float>(1.0f - pUVs.mData[1]);

				// Position
				Temp.Pos.x = CurrCtrlPoint->mPosition.x;
				Temp.Pos.y = CurrCtrlPoint->mPosition.y;
				Temp.Pos.z = CurrCtrlPoint->mPosition.z;
				VeetexData.insert(controlPointIndex, Temp);
			}
		}
	}
	for (int i = 0; i < VeetexData.size(); ++i) {
		Vertex ver_base = VeetexData[i];
		sf->add_normal(ver_base.Normal);
		if (uv_count > 0) {
			sf->add_uv(ver_base.TexC);
		}
		if (uv_count > 1) {
			sf->add_uv2(ver_base.TexC2);
		}
		if (uv_count > 2) {
			sf->add_uv3(ver_base.TexC3);
		}
		if (uv_count > 3) {
			sf->add_uv4(ver_base.TexC4);
		}
		if (has_color) {
			sf->add_color(color[i]);
		}
		if (bone_weight_data) {
			Vector<int> bone_index;
			Vector<float> bone_weight;
			CtrlPoint *cp = bone_weight_data->mControlPoints[i];
			for (int b = 0; b < cp->mBoneInfo.size(); ++b) {
				bone_index.push_back(cp->mBoneInfo[b].mBoneIndices);
				bone_weight.push_back(cp->mBoneInfo[b].mBoneWeight);
			}
			sf->add_bones(bone_index);
			sf->add_weights(bone_weight);
		}
		sf->add_vertex(ver_base.Pos);
	}
}

void FbxLoader::GetAnimation(
		fbxsdk::FbxScene *pFbxScene,
		fbxsdk::FbxNode *pFbxChildNode, MeshInstance *meshInstance,
		const String &ClipName, bool isGetOnlyAnim) {
	fbxsdk::FbxMesh *pMesh = (fbxsdk::FbxMesh *)pFbxChildNode->GetNodeAttribute();
	FbxAMatrix geometryTransform = GetGeometryTransformation(pFbxChildNode);

	// Deformer - Cluster - Link
	// Deformer
	// 骨架节点信息
	Skeleton *SekeletedNode = nullptr;
	// 骨架加载信息
	SkinnedLoadData *skindata = nullptr;
	// 模型加载数据
	MeshLoadData *meshdata = GetMeshLoadData(pFbxChildNode);
	// 加载猛批信息
	for (int deformerIndex = 0; deformerIndex < pMesh->GetDeformerCount(); ++deformerIndex) {
		FbxSkin *pCurrSkin = reinterpret_cast<FbxSkin *>(pMesh->GetDeformer(deformerIndex, FbxDeformer::eSkin));
		if (!pCurrSkin) {
			continue;
		}

		// Cluster
		for (int clusterIndex = 0; clusterIndex < pCurrSkin->GetClusterCount(); ++clusterIndex) {
			FbxCluster *pCurrCluster = pCurrSkin->GetCluster(clusterIndex);

			skindata = GetSkinnedLoadData(pCurrCluster->GetLink());
			if (skindata == nullptr) {
				continue;
			}
			meshdata->skin.instance();
			SekeletedNode = (Skeleton *)GetSceneNode(pCurrCluster->GetLink());
			meshdata->pSekeleton = SekeletedNode;
			// To find the index that matches the name of the current joint
			String currJointName = pCurrCluster->GetLink()->GetName();
			uint8_t currJointIndex; // current joint index
			for (currJointIndex = 0; currJointIndex < skindata->mBoneName.size(); ++currJointIndex) {
				if (skindata->mBoneName[currJointIndex] == currJointName)
					break;
			}

			{
				MeshBoneWeightData *meshBoneWeight = GetMeshBoneWeightData(pFbxChildNode);
				if (meshBoneWeight == nullptr) {
					meshBoneWeight = memnew(MeshBoneWeightData);
					MeshBoneWeight.insert(pFbxChildNode, meshBoneWeight);
				}
				FbxAMatrix transformMatrix, transformLinkMatrix;
				FbxAMatrix globalBindposeInverseMatrix;

				transformMatrix = pCurrCluster->GetTransformMatrix(transformMatrix); // The transformation of the mesh at binding time
				transformLinkMatrix = pCurrCluster->GetTransformLinkMatrix(transformLinkMatrix); // The transformation of the cluster(joint) at binding time from joint space to world space
				globalBindposeInverseMatrix = transformLinkMatrix.Inverse() * transformMatrix * geometryTransform;

				// Set the BoneOffset Matrix
				Transform boneOffset;

				FbxVector4 T = globalBindposeInverseMatrix.GetT();
				FbxQuaternion Q = globalBindposeInverseMatrix.GetQ();
				FbxVector4 S = globalBindposeInverseMatrix.GetS();
				boneOffset.origin = Vector3(T[0], T[1], T[2]);
				boneOffset.basis.set_quat_scale(Quat(Q[0], Q[1], Q[2], Q[3]), Vector3(S[0], S[1], S[2]));
				skindata->mBoneOffsets[currJointIndex] = boneOffset;
				// 设置蒙皮信息
				meshdata->skin->add_named_bind(currJointName, boneOffset);

				// Set the Bone index and weight ./ Max 4
				int *controlPointIndices = pCurrCluster->GetControlPointIndices();
				for (int i = 0; i < pCurrCluster->GetControlPointIndicesCount(); ++i) {
					BoneIndexAndWeight currBoneIndexAndWeight;
					currBoneIndexAndWeight.mBoneIndices = currJointIndex;
					currBoneIndexAndWeight.mBoneWeight = pCurrCluster->GetControlPointWeights()[i];

					meshBoneWeight->mControlPoints[controlPointIndices[i]]->mBoneInfo.push_back(currBoneIndexAndWeight);
					meshBoneWeight->mControlPoints[controlPointIndices[i]]->mBoneName = currJointName;
				}
			}

			// Set the Bone Animation Matrix
			BoneAnimation boneAnim;
			FbxAnimStack *pCurrAnimStack = pFbxScene->GetSrcObject<FbxAnimStack>(0);
			FbxAnimEvaluator *pSceneEvaluator = pFbxScene->GetAnimationEvaluator();

			SekeletonAnimationData *anim = GetSkinnedAnimationData(SekeletedNode);
			if (anim == nullptr) {
				anim = memnew(SekeletonAnimationData);
			}
			// Animation Data
			AnimationClip *animation = nullptr;
			if (anim->mAnimations.has(ClipName)) {
				animation = anim->mAnimations.find(ClipName)->value();
			} else {
				animation = memnew(AnimationClip);
				// Initialize BoneAnimations
				animation->BoneAnimations.resize(skindata->mBoneName.size());
				anim->mAnimations.insert(ClipName, animation);
			}

			// TRqS transformation and Time per frame
			FbxLongLong index;
			for (index = 0; index < 100; ++index) {
				FbxTime currTime;
				currTime.SetFrame(index, FbxTime::eCustom);

				Keyframe key;
				key.TimePos = static_cast<float>(index) / 8.0f;

				FbxAMatrix currentTransformOffset = pSceneEvaluator->GetNodeGlobalTransform(pFbxChildNode, currTime) * geometryTransform;
				FbxAMatrix temp = currentTransformOffset.Inverse() * pSceneEvaluator->GetNodeGlobalTransform(pCurrCluster->GetLink(), currTime);

				// Transition, Scaling and Rotation Quaternion
				FbxVector4 TS = temp.GetT();
				key.Translation = {
					static_cast<float>(TS.mData[0]),
					static_cast<float>(TS.mData[1]),
					static_cast<float>(TS.mData[2])
				};
				TS = temp.GetS();
				key.Scale = {
					static_cast<float>(TS.mData[0]),
					static_cast<float>(TS.mData[1]),
					static_cast<float>(TS.mData[2])
				};
				FbxQuaternion Q = temp.GetQ();
				key.RotationQuat = {
					static_cast<float>(Q.mData[0]),
					static_cast<float>(Q.mData[1]),
					static_cast<float>(Q.mData[2]),
					static_cast<float>(Q.mData[3])
				};

				// Frame does not exist
				if (index != 0 && boneAnim.Keyframes.back() == key)
					break;

				boneAnim.Keyframes.push_back(key);
			}
			animation->BoneAnimations[currJointIndex] = boneAnim;
		}
	}
	// 家在变形信息
	for (int deformerIndex = 0; deformerIndex < pMesh->GetDeformerCount(); ++deformerIndex) {
		FbxBlendShape *pCurrShape = reinterpret_cast<FbxBlendShape *>(pMesh->GetDeformer(deformerIndex, FbxDeformer::eBlendShape));
		if (pCurrShape) {
			String BlendShapeName = pCurrShape->GetName();
			const int32_t BlendShapeChannelCount = pCurrShape->GetBlendShapeChannelCount();
			for (int32_t ChannelIndex = 0; ChannelIndex < BlendShapeChannelCount; ++ChannelIndex) {
				FbxBlendShapeChannel *Channel = pCurrShape->GetBlendShapeChannel(ChannelIndex);

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
							GetVerticesAndIndice(pFbxChildNode, meshShape, morph_st);
							meshdata->morphs.push_back(morph_st);
						}
					}
				}
			}

			continue;
		}
	}
}

Spatial *FbxLoader::LoadFBX(
		String fileName) {
	// if exported animation exist

	if (gFbxManager == nullptr) {
		gFbxManager = FbxManager::Create();

		FbxIOSettings *pIOsettings = FbxIOSettings::Create(gFbxManager, IOSROOT);
		gFbxManager->SetIOSettings(pIOsettings);
	}

	std::vector<CharacterVertex> outVertexVector;
	std::vector<uint32_t> outIndexVector;
	SkinnedData outSkinnedData;
	String clipName = "RootAnim";
	std::vector<Material> outMaterial;
	FbxImporter *pImporter = FbxImporter::Create(gFbxManager, "");
	String g_path = ProjectSettings::get_singleton()->globalize_path(fileName);
	String fbxFileName = g_path;
	bool bSuccess = pImporter->Initialize(fbxFileName.utf8(), -1, gFbxManager->GetIOSettings());
	if (!bSuccess)
		return nullptr;

	FbxScene *pFbxScene = FbxScene::Create(gFbxManager, "");
	bSuccess = pImporter->Import(pFbxScene);
	if (!bSuccess)
		return nullptr;

	pImporter->Destroy();
	Spatial *root_node = nullptr;
	FbxAxisSystem sceneAxisSystem = pFbxScene->GetGlobalSettings().GetAxisSystem();
	FbxAxisSystem::MayaZUp.ConvertScene(pFbxScene); // Delete?

	// Convert quad to triangle
	FbxGeometryConverter geometryConverter(gFbxManager);
	geometryConverter.Triangulate(pFbxScene, true);

	// Start to RootNode
	fbxsdk::FbxNode *pFbxRootNode = pFbxScene->GetRootNode();
	if (pFbxRootNode) {
		RootNode = memnew(Spatial);
		// 解析所有的骨架信息
		ProcessSkeletonHierarchy(pFbxRootNode, RootNode);
		// Bone offset, Control point, Vertex, Index Data
		// And Animation Data
		int ii = pFbxRootNode->GetChildCount();
		ProcessMeshAndAnimation(pFbxScene, pFbxRootNode, RootNode);
		outSkinnedData.Set(mBoneHierarchy, mBoneOffsets, &mAnimations);
	}
	// 初始化所有的模型信息
	Map<fbxsdk::FbxNode *, MeshLoadData *>::Element *mesh_loads = TotalMeshLoadDataMap.front();
	while (mesh_loads) {
		MeshLoadData *data = mesh_loads->value();
		MeshInstance *mesh_instance = data->MeshIns;
		if (mesh_instance == nullptr) {
			continue;
		}

		Array array_mesh = data->vertex->commit_to_arrays();
		Array morphs;
		Ref<ArrayMesh> mesh;
		for (int i = 0; i < data->morphs.size(); ++i) {
			Ref<SurfaceTool> m = data->morphs[i];
			morphs.append(m->commit_to_arrays());
		}
		mesh.instance();
		Mesh::PrimitiveType primitive = Mesh::PRIMITIVE_TRIANGLES;
		uint32_t mesh_flags = Mesh::ARRAY_COMPRESS_DEFAULT;
		mesh->add_surface_from_arrays(primitive, array_mesh, morphs, mesh_flags);
		mesh->set_name(data->MeshName);
		mesh->surface_set_material(0, data->GetMaterial(0));
		if (data->pSekeleton) {
			data->pSekeleton->add_child(mesh_instance);
			mesh_instance->set_global_transform(Transform());
			mesh_instance->set_mesh(mesh);
			mesh_instance->set_skeleton_path(mesh_instance->get_path_to(data->pSekeleton));
			mesh_instance->set_skin(data->skin);
		} else {
			Transform globle_trans = FbxMatrixToTransform(mesh_loads->key()->EvaluateGlobalTransform());
			Spatial *parentNode = GetSceneNode(mesh_loads->key()->GetParent());
			parentNode->add_child(mesh_instance);
			mesh_instance->set_global_transform(globle_trans);
		}

		mesh_loads = mesh_loads->next();
	}
	return root_node;
}

void FbxLoader::GetControlPoints(fbxsdk::FbxNode *pFbxRootNode) {
	fbxsdk::FbxMesh *pCurrMesh = (fbxsdk::FbxMesh *)pFbxRootNode->GetNodeAttribute();

	unsigned int ctrlPointCount = pCurrMesh->GetControlPointsCount();
	for (unsigned int i = 0; i < ctrlPointCount; ++i) {
		CtrlPoint *currCtrlPoint = new CtrlPoint();

		Vector3 currPosition;
		currPosition.x = static_cast<float>(pCurrMesh->GetControlPointAt(i).mData[0]);
		currPosition.y = static_cast<float>(pCurrMesh->GetControlPointAt(i).mData[1]);
		currPosition.z = static_cast<float>(pCurrMesh->GetControlPointAt(i).mData[2]);

		currCtrlPoint->mPosition = currPosition;
		mControlPoints[i] = currCtrlPoint;
	}
}
void FbxLoader::GetAnimation(
		fbxsdk::FbxScene *pFbxScene,
		fbxsdk::FbxNode *pFbxChildNode,
		SkinnedData &outSkinnedData,
		const String &ClipName,
		bool isGetOnlyAnim) {
	fbxsdk::FbxMesh *pMesh = (fbxsdk::FbxMesh *)pFbxChildNode->GetNodeAttribute();
	FbxAMatrix geometryTransform = GetGeometryTransformation(pFbxChildNode);

	// Animation Data
	AnimationClip animation;

	// Initialize BoneAnimations
	animation.BoneAnimations.resize(mBoneName.size());

	// Deformer - Cluster - Link
	// Deformer
	for (int deformerIndex = 0; deformerIndex < pMesh->GetDeformerCount(); ++deformerIndex) {
		FbxSkin *pCurrSkin = reinterpret_cast<FbxSkin *>(pMesh->GetDeformer(deformerIndex, FbxDeformer::eSkin));
		if (!pCurrSkin) {
			continue;
		}

		// Cluster
		for (int clusterIndex = 0; clusterIndex < pCurrSkin->GetClusterCount(); ++clusterIndex) {
			FbxCluster *pCurrCluster = pCurrSkin->GetCluster(clusterIndex);

			// To find the index that matches the name of the current joint
			String currJointName = pCurrCluster->GetLink()->GetName();
			uint8_t currJointIndex; // current joint index
			for (currJointIndex = 0; currJointIndex < mBoneName.size(); ++currJointIndex) {
				if (mBoneName[currJointIndex] == currJointName)
					break;
			}

			if (!isGetOnlyAnim) {
				FbxAMatrix transformMatrix, transformLinkMatrix;
				FbxAMatrix globalBindposeInverseMatrix;

				transformMatrix = pCurrCluster->GetTransformMatrix(transformMatrix); // The transformation of the mesh at binding time
				transformLinkMatrix = pCurrCluster->GetTransformLinkMatrix(transformLinkMatrix); // The transformation of the cluster(joint) at binding time from joint space to world space
				globalBindposeInverseMatrix = transformLinkMatrix.Inverse() * transformMatrix * geometryTransform;

				// Set the BoneOffset Matrix
				Transform boneOffset;

				FbxVector4 T = globalBindposeInverseMatrix.GetT();
				FbxQuaternion Q = globalBindposeInverseMatrix.GetQ();
				FbxVector4 S = globalBindposeInverseMatrix.GetS();
				boneOffset.origin = Vector3(T[0], T[1], T[2]);
				boneOffset.basis.set_quat_scale(Quat(Q[0], Q[1], Q[2], Q[3]), Vector3(S[0], S[1], S[2]));
				mBoneOffsets[currJointIndex] = boneOffset;

				// Set the Bone index and weight ./ Max 4
				int *controlPointIndices = pCurrCluster->GetControlPointIndices();
				for (int i = 0; i < pCurrCluster->GetControlPointIndicesCount(); ++i) {
					BoneIndexAndWeight currBoneIndexAndWeight;
					currBoneIndexAndWeight.mBoneIndices = currJointIndex;
					currBoneIndexAndWeight.mBoneWeight = pCurrCluster->GetControlPointWeights()[i];

					mControlPoints[controlPointIndices[i]]->mBoneInfo.push_back(currBoneIndexAndWeight);
					mControlPoints[controlPointIndices[i]]->mBoneName = currJointName;
				}
			}

			// Set the Bone Animation Matrix
			BoneAnimation boneAnim;
			FbxAnimStack *pCurrAnimStack = pFbxScene->GetSrcObject<FbxAnimStack>(0);
			FbxAnimEvaluator *pSceneEvaluator = pFbxScene->GetAnimationEvaluator();

			// TRqS transformation and Time per frame
			FbxLongLong index;
			for (index = 0; index < 100; ++index) {
				FbxTime currTime;
				currTime.SetFrame(index, FbxTime::eCustom);

				Keyframe key;
				key.TimePos = static_cast<float>(index) / 8.0f;

				FbxAMatrix currentTransformOffset = pSceneEvaluator->GetNodeGlobalTransform(pFbxChildNode, currTime) * geometryTransform;
				FbxAMatrix temp = currentTransformOffset.Inverse() * pSceneEvaluator->GetNodeGlobalTransform(pCurrCluster->GetLink(), currTime);

				// Transition, Scaling and Rotation Quaternion
				FbxVector4 TS = temp.GetT();
				key.Translation = {
					static_cast<float>(TS.mData[0]),
					static_cast<float>(TS.mData[1]),
					static_cast<float>(TS.mData[2])
				};
				TS = temp.GetS();
				key.Scale = {
					static_cast<float>(TS.mData[0]),
					static_cast<float>(TS.mData[1]),
					static_cast<float>(TS.mData[2])
				};
				FbxQuaternion Q = temp.GetQ();
				key.RotationQuat = {
					static_cast<float>(Q.mData[0]),
					static_cast<float>(Q.mData[1]),
					static_cast<float>(Q.mData[2]),
					static_cast<float>(Q.mData[3])
				};

				// Frame does not exist
				if (index != 0 && boneAnim.Keyframes.back() == key)
					break;

				boneAnim.Keyframes.push_back(key);
			}
			animation.BoneAnimations[currJointIndex] = boneAnim;
		}
	}

	BoneAnimation InitBoneAnim;

	// Initialize InitBoneAnim
	for (int i = 0; i < mBoneName.size(); ++i) {
		int KeyframeSize = animation.BoneAnimations[i].Keyframes.size();
		if (KeyframeSize != 0) {
			for (int j = 0; j < KeyframeSize; ++j) // 60 frames
			{
				Keyframe key;

				key.TimePos = static_cast<float>(j / 24.0f);
				key.Translation = { 0.0f, 0.0f, 0.0f };
				key.Scale = { 1.0f, 1.0f, 1.0f };
				key.RotationQuat = { 0.0f, 0.0f, 0.0f, 0.0f };
				InitBoneAnim.Keyframes.push_back(key);
			}
			break;
		}
	}

	for (int i = 0; i < mBoneName.size(); ++i) {
		if (animation.BoneAnimations[i].Keyframes.size() != 0)
			continue;

		animation.BoneAnimations[i] = InitBoneAnim;
	}

	if (!isGetOnlyAnim) {
		BoneIndexAndWeight currBoneIndexAndWeight;
		currBoneIndexAndWeight.mBoneIndices = 0;
		currBoneIndexAndWeight.mBoneWeight = 0;
		for (auto itr = mControlPoints.begin(); itr != mControlPoints.end(); ++itr) {
			for (uint32_t i = itr->second->mBoneInfo.size(); i <= 4; ++i) {
				itr->second->mBoneInfo.push_back(currBoneIndexAndWeight);
			}
		}

		mAnimations[ClipName] = animation;
	}

	outSkinnedData.SetAnimation(animation, ClipName);
}

void FbxLoader::GetMaterials(FbxNode *pNode, std::vector<FbxMaterial> &outMaterial) {
	int MaterialCount = pNode->GetMaterialCount();

	for (int i = 0; i < MaterialCount; ++i) {
		FbxMaterial tempMaterial;
		FbxSurfaceMaterial *SurfaceMaterial = pNode->GetMaterial(i);
		GetMaterialAttribute(SurfaceMaterial, tempMaterial);
		GetMaterialTexture(SurfaceMaterial, tempMaterial);

		if (tempMaterial.Name != "") {
			outMaterial.push_back(tempMaterial);
		}
	}
}

void FbxLoader::GetMaterialAttribute(FbxSurfaceMaterial *pMaterial, FbxMaterial &outMaterial) {
	FbxDouble3 double3;
	FbxDouble double1;
	if (pMaterial->GetClassId().Is(FbxSurfacePhong::ClassId)) {
		// Amibent Color
		double3 = reinterpret_cast<FbxSurfacePhong *>(pMaterial)->Ambient;
		outMaterial.Ambient.r = static_cast<float>(double3.mData[0]);
		outMaterial.Ambient.g = static_cast<float>(double3.mData[1]);
		outMaterial.Ambient.b = static_cast<float>(double3.mData[2]);

		// Diffuse Color
		double3 = reinterpret_cast<FbxSurfacePhong *>(pMaterial)->Diffuse;
		outMaterial.DiffuseAlbedo.r = static_cast<float>(double3.mData[0]);
		outMaterial.DiffuseAlbedo.g = static_cast<float>(double3.mData[1]);
		outMaterial.DiffuseAlbedo.b = static_cast<float>(double3.mData[2]);

		// Roughness
		double1 = reinterpret_cast<FbxSurfacePhong *>(pMaterial)->Shininess;
		outMaterial.Roughness = 1 - double1;

		// Reflection
		double3 = reinterpret_cast<FbxSurfacePhong *>(pMaterial)->Reflection;
		outMaterial.FresnelR0.r = static_cast<float>(double3.mData[0]);
		outMaterial.FresnelR0.g = static_cast<float>(double3.mData[1]);
		outMaterial.FresnelR0.b = static_cast<float>(double3.mData[2]);

		// Specular Color
		double3 = reinterpret_cast<FbxSurfacePhong *>(pMaterial)->Specular;
		outMaterial.Specular.r = static_cast<float>(double3.mData[0]);
		outMaterial.Specular.g = static_cast<float>(double3.mData[1]);
		outMaterial.Specular.b = static_cast<float>(double3.mData[2]);

		// Emissive Color
		double3 = reinterpret_cast<FbxSurfacePhong *>(pMaterial)->Emissive;
		outMaterial.Emissive.r = static_cast<float>(double3.mData[0]);
		outMaterial.Emissive.g = static_cast<float>(double3.mData[1]);
		outMaterial.Emissive.b = static_cast<float>(double3.mData[2]);

		/*
		// Transparency Factor
		double1 = reinterpret_cast<FbxSurfacePhong *>(inMaterial)->TransparencyFactor;
		currMaterial->mTransparencyFactor = double1;

		// Specular Factor
		double1 = reinterpret_cast<FbxSurfacePhong *>(inMaterial)->SpecularFactor;
		currMaterial->mSpecularPower = double1;


		// Reflection Factor
	double1 = reinterpret_cast<FbxSurfacePhong *>(inMaterial)->ReflectionFactor;
		currMaterial->mReflectionFactor = double1;	*/
	} else if (pMaterial->GetClassId().Is(FbxSurfaceLambert::ClassId)) {
		// Amibent Color
		double3 = reinterpret_cast<FbxSurfaceLambert *>(pMaterial)->Ambient;
		outMaterial.Ambient.r = static_cast<float>(double3.mData[0]);
		outMaterial.Ambient.g = static_cast<float>(double3.mData[1]);
		outMaterial.Ambient.b = static_cast<float>(double3.mData[2]);

		// Diffuse Color
		double3 = reinterpret_cast<FbxSurfaceLambert *>(pMaterial)->Diffuse;
		outMaterial.DiffuseAlbedo.r = static_cast<float>(double3.mData[0]);
		outMaterial.DiffuseAlbedo.g = static_cast<float>(double3.mData[1]);
		outMaterial.DiffuseAlbedo.b = static_cast<float>(double3.mData[2]);

		// Emissive Color
		double3 = reinterpret_cast<FbxSurfaceLambert *>(pMaterial)->Emissive;
		outMaterial.Emissive.r = static_cast<float>(double3.mData[0]);
		outMaterial.Emissive.g = static_cast<float>(double3.mData[1]);
		outMaterial.Emissive.b = static_cast<float>(double3.mData[2]);
	}
}

void FbxLoader::GetMaterialTexture(fbxsdk::FbxSurfaceMaterial *pMaterial, FbxMaterial &Mat) {
	unsigned int textureIndex = 0;
	FbxProperty property;

	FBXSDK_FOR_EACH_TEXTURE(textureIndex) {
		property = pMaterial->FindProperty(FbxLayerElement::sTextureChannelNames[textureIndex]);
		if (property.IsValid()) {
			unsigned int textureCount = property.GetSrcObjectCount<FbxTexture>();
			for (unsigned int i = 0; i < textureCount; ++i) {
				FbxLayeredTexture *layeredTexture = property.GetSrcObject<FbxLayeredTexture>(i);
				if (layeredTexture) {
				} else {
					FbxTexture *texture = property.GetSrcObject<FbxTexture>(i);
					if (texture) {
						std::string textureType = property.GetNameAsCStr();
						FbxFileTexture *fileTexture = FbxCast<FbxFileTexture>(texture);

						if (fileTexture) {
							if (textureType == "DiffuseColor") {
								Mat.Name = fileTexture->GetFileName();
							}
							/*else if (textureType == "SpecularColor")
							{
							Mat->mSpecularMapName = fileTexture->GetFileName();
							}
							else if (textureType == "Bump")
							{
							Mat->mNormalMapName = fileTexture->GetFileName();
							}*/
						}
					}
				}
			}
		}
	}
}

FbxAMatrix FbxLoader::GetGeometryTransformation(FbxNode *pNode) {
	if (!pNode) {
		print_error("Null for mesh geometry");
	}

	const FbxVector4 lT = pNode->GetGeometricTranslation(FbxNode::eSourcePivot);
	const FbxVector4 lR = pNode->GetGeometricRotation(FbxNode::eSourcePivot);
	const FbxVector4 lS = pNode->GetGeometricScaling(FbxNode::eSourcePivot);

	return FbxAMatrix(lT, lR, lS);
}

void FbxLoader::clear() {
	mControlPoints.clear();
	mBoneName.clear();
	mBoneHierarchy.clear();
	mBoneOffsets.clear();
	mAnimations.clear();
}