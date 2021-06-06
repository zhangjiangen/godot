#include "fbx_utils.hpp"

#include <algorithm>
#include <iostream>

#ifdef IOS_REF
#undef IOS_REF
#define IOS_REF (*(g_manager->GetIOSettings()))
#endif

// =============================================================================
namespace Fbx_utils {
// =============================================================================

void print_ver(int *v) {
	std::cout << v[0] << "." << v[1] << "." << v[2] << std::endl;
}

// -----------------------------------------------------------------------------

bool load_scene(const String &filename,
		FbxScene *fbx_scene,
		FbxManager *g_manager) {
	const char *cfile = filename.utf8();
	int file_ver[3];
	int sdk_ver[3];

	// Initialize Fbx loader

	// Get the file version number generate by the FBX SDK.
	FbxManager::GetFileFormatVersion(sdk_ver[0], sdk_ver[1], sdk_ver[2]);
	std::cout << "FBX version number for this FBX SDK is ";
	print_ver(sdk_ver);

	// Create an importer.
	FbxImporter *importer = FbxImporter::Create(g_manager, "");

	// Initialize the importer by providing a filename.
	const bool import_state = importer->Initialize(cfile, -1, &(IOS_REF));

	// Get file version for the loaded file
	importer->GetFileVersion(file_ver[0], file_ver[1], file_ver[2]);
	std::cout << "FBX version number for file " << cfile << " is ";
	print_ver(file_ver);

	if (!import_state) {
		std::cout << "Call to FbxImporter::Initialize() failed.\n";
		std::cout << "Error returned: " << importer->GetStatus().GetErrorString();
		std::cout << "\n\n";
		return false;
	}

	if (importer->IsFBX()) {
		// From this point, it is possible to access animation stack information
		// without the expense of loading the entire file.

		std::cout << "Animation Stack Information\n";
		int stack_len = importer->GetAnimStackCount();
		std::cout << "    Number of Animation Stacks: " << stack_len << "\n";
		std::cout << "    Current Animation Stack: \"";
		std::cout << importer->GetActiveAnimStackName().Buffer() << "\"\n";
		std::cout << std::endl;
		for (int i = 0; i < stack_len; i++) {
			FbxTakeInfo *info = importer->GetTakeInfo(i);
			std::cout << "    Animation Stack " << i << "\n";
			std::cout << "         Name: \"" << info->mName.Buffer() << "\"\n";
			std::cout << "         Description: \"" << info->mDescription.Buffer();
			std::cout << "\"\n";

			// Change the value of the import name if the animation stack should
			// be imported under a different name.

			std::cout << "         Import Name: \"" << info->mImportName.Buffer();
			std::cout << "\"\n";

			// Set the value of the import state to false if the animation stack
			// should be not be imported.
			const char *tmp = info->mSelect ? "true" : "false";
			std::cout << "         Import State: " << tmp << "\n";
			std::cout << std::endl;
		}
		// Set the import states. By default, the import states are always set to
		// true. The code below shows how to change these states.
		IOS_REF.SetBoolProp(IMP_FBX_MATERIAL, true);
		IOS_REF.SetBoolProp(IMP_FBX_TEXTURE, true);
		IOS_REF.SetBoolProp(IMP_FBX_LINK, true);
		IOS_REF.SetBoolProp(IMP_FBX_SHAPE, true);
		IOS_REF.SetBoolProp(IMP_FBX_GOBO, true);
		IOS_REF.SetBoolProp(IMP_FBX_ANIMATION, true);
		IOS_REF.SetBoolProp(IMP_FBX_GLOBAL_SETTINGS, true);
	}

	// Import the scene
	bool status = importer->Import(fbx_scene);
	// deal with protected files
	if (status == false && importer->GetStatus().GetCode() == FbxStatus::ePasswordError) {
		char password[1024];
		std::cout << "Please enter password: ";
		password[0] = '\0';
		std::cin >> password;
		FbxString string(password);
		IOS_REF.SetStringProp(IMP_FBX_PASSWORD, string);
		IOS_REF.SetBoolProp(IMP_FBX_PASSWORD_ENABLE, true);
		status = importer->Import(fbx_scene);

		if (!status && importer->GetStatus().GetCode() == FbxStatus::ePasswordError) {
			std::cout << "\nPassword is wrong, import aborted." << std::endl;
		}
	} else {
		std::cerr << importer->GetStatus().GetErrorString();
	}

	// Destroy the importer.
	importer->Destroy();

	return status;
}

// -----------------------------------------------------------------------------

FbxNode *find_root(FbxNode *root, FbxNodeAttribute::EType attrib) {
	unsigned nb_children = root->GetChildCount();
	if (!nb_children)
		return 0;

	FbxNode *child = 0;
	for (unsigned c = 0; c < nb_children; c++) {
		child = root->GetChild(c);
		if (!child->GetNodeAttribute())
			continue;
		if (child->GetNodeAttribute()->GetAttributeType() != attrib)
			continue;

		return child;
	}

	FbxNode *joint = 0;
	for (unsigned c = 0; c < nb_children; c++) {
		child = root->GetChild(c);
		joint = find_root(child, attrib);
		if (joint)
			break;
	}
	return joint;
}

// -----------------------------------------------------------------------------

void print_hierarchy(FbxNode *pNode, int pDepth);

void print_hierarchy(FbxScene *pScene) {
	int i;
	FbxNode *lRootNode = pScene->GetRootNode();

	for (i = 0; i < lRootNode->GetChildCount(); i++) {
		print_hierarchy(lRootNode->GetChild(i), 0);
	}
}

// -----------------------------------------------------------------------------

void print_hierarchy(FbxNode *pNode, int pDepth) {
	String string;
	int i;

	for (i = 0; i < pDepth; i++)
		string += "     ";

	string += pNode->GetName();
	string += " attr type: ";
	string += to_string(pNode->GetNodeAttribute()->GetAttributeType());

	std::cout << string.utf8() << std::endl;

	for (i = 0; i < pNode->GetChildCount(); i++)
		print_hierarchy(pNode->GetChild(i), pDepth + 1);
}

// -----------------------------------------------------------------------------

void print_anim_stacks(FbxScene *scene) {
	for (int i = 0; i < scene->GetSrcObjectCount<FbxAnimStack>(); i++) {
		FbxAnimStack *stack = FbxCast<FbxAnimStack>(scene->GetSrcObject<FbxAnimStack>(i));

		FbxString str = "Animation Stack Name: ";
		str += stack->GetName();
		str += "\n";
		std::cout << str << std::endl;
	}
}

// -----------------------------------------------------------------------------

void set_global_frame(FbxNode *pNode, FbxMatrix pGlobalPosition) {
	FbxMatrix lLocalPosition;
	FbxMatrix lParentGlobalPosition;

	if (pNode->GetParent()) {
		lParentGlobalPosition = get_global_frame(pNode->GetParent());
		lLocalPosition = lParentGlobalPosition.Inverse() * pGlobalPosition;
	} else {
		lLocalPosition = pGlobalPosition;
	}

	/* Old code FBX 2011
    pNode->LclTranslation.Set(lLocalPosition.GetT());
    pNode->LclRotation.Set(lLocalPosition.GetR());
    pNode->LclScaling.Set(lLocalPosition.GetS());
    */
	std::cout << "code to be tested" << std::endl;
	FbxVector4 tr, rot, shear, scale;
	double sign;
	lLocalPosition.GetElements(tr, rot, shear, scale, sign);
	pNode->LclTranslation.Set(tr);
	pNode->LclRotation.Set(rot);
	pNode->LclScaling.Set(scale);
}

// -----------------------------------------------------------------------------

FbxMatrix get_global_frame(const FbxNode *pNode) {
	FbxMatrix lLocalPosition;
	FbxMatrix lGlobalPosition;
	FbxMatrix lParentGlobalPosition;

	lLocalPosition.SetTRS(pNode->LclTranslation.Get(),
			pNode->LclRotation.Get(),
			pNode->LclScaling.Get());

	if (pNode->GetParent()) {
		lParentGlobalPosition = get_global_frame(pNode->GetParent());
		lGlobalPosition = lParentGlobalPosition * lLocalPosition;
	} else {
		lGlobalPosition = lLocalPosition;
	}

	return lGlobalPosition;
}

// -----------------------------------------------------------------------------

FbxAMatrix geometry_transfo(FbxNode *pNode) {
	FbxVector4 lT, lR, lS;
	FbxAMatrix tr;

	lT = pNode->GetGeometricTranslation(FbxNode::eSourcePivot);
	lR = pNode->GetGeometricRotation(FbxNode::eSourcePivot);
	lS = pNode->GetGeometricScaling(FbxNode::eSourcePivot);

	tr.SetTRS(lT, lR, lS);
	return tr;
}

// -----------------------------------------------------------------------------

void copy(float *t, const FbxDouble3 &d3) {
	t[0] = (float)d3[0];
	t[1] = (float)d3[1];
	t[2] = (float)d3[2];
}

// -----------------------------------------------------------------------------

void copy(Loader::Material &m, const FbxSurfacePhong *phong) {
	m._name = String(phong->GetName());
	Fbx_utils::copy(m._Ka, phong->Ambient.Get());
	Fbx_utils::copy(m._Kd, phong->Diffuse.Get());
	Fbx_utils::copy(m._Ks, phong->Specular.Get());
	m._Tf[0] = m._Tf[1] = m._Tf[2] = std::max(1.f - (float)phong->TransparencyFactor.Get(), 0.f);
	m._Ns = (float)phong->Shininess.Get();
}

// -----------------------------------------------------------------------------

void copy(Loader::Material &m, const FbxSurfaceLambert *lbrt) {
	m._name = String(lbrt->GetName());
	Fbx_utils::copy(m._Ka, lbrt->Ambient.Get());
	Fbx_utils::copy(m._Kd, lbrt->Diffuse.Get());
	m._Tf[0] = m._Tf[1] = m._Tf[2] = std::max(1.f - (float)lbrt->TransparencyFactor.Get(), 0.f);

	// textures
}

// -----------------------------------------------------------------------------

Transform to_transfo(const FbxMatrix mat) {
	Transform tr;
	tr.basis.set(mat[0][0], mat[0][1], mat[0][2],
			mat[1][0], mat[1][1], mat[1][2],
			mat[2][0], mat[2][1], mat[2][2]);
	tr.origin.x = mat[0][3];
	tr.origin.y = mat[1][3];
	tr.origin.z = mat[2][3];

	return tr;
}

// -----------------------------------------------------------------------------

Color to_color(const FbxColor &c) {
	return Color(c.mRed, c.mGreen, c.mBlue, c.mAlpha);
}
Transform to_transfo(const FbxMatrix &mat) {
	Transform tr;
	tr.basis.set(mat[0][0], mat[0][1], mat[0][2],
			mat[1][0], mat[1][1], mat[1][2],
			mat[2][0], mat[2][1], mat[2][2]);
	tr.origin.x = mat[0][3];
	tr.origin.y = mat[1][3];
	tr.origin.z = mat[2][3];

	return tr;
}

// -----------------------------------------------------------------------------

Vector3 to_lvertex(const FbxVector4 &vec) {
	return Vector3((float)vec[0], (float)vec[1], (float)vec[2]);
}

// -----------------------------------------------------------------------------

Vector3 to_lnormal(const FbxVector4 &vec) {
	return Vector3((float)vec[0], (float)vec[1], (float)vec[2]);
}

// -----------------------------------------------------------------------------

Vector2 to_ltexcoord(const FbxVector2 &vec) {
	return Vector2((float)vec[0], (float)vec[1]);
}

// -----------------------------------------------------------------------------

String to_string(FbxGeometryElement::EMappingMode type) {
	String str;
	switch (type) {
		case (FbxGeometryElement::eNone):
			str = "eNONE";
			break;
		case (FbxGeometryElement::eByControlPoint):
			str = "eBY_CONTROL_POINT";
			break;
		case (FbxGeometryElement::eByPolygonVertex):
			str = "eBY_POLYGON_VERTEX";
			break;
		case (FbxGeometryElement::eByPolygon):
			str = "eBY_POLYGON";
			break;
		case (FbxGeometryElement::eByEdge):
			str = "eBY_EDGE";
			break;
		case (FbxGeometryElement::eAllSame):
			str = "eALL_SAME";
			break;
	}

	return str;
}

// -----------------------------------------------------------------------------

String to_string(FbxNodeAttribute::EType type) {
	String str;
	switch (type) {
		case (FbxNodeAttribute::eUnknown):
			str = "eUnknown";
			break;
		case (FbxNodeAttribute::eNull):
			str = "eNULL";
			break;
		case (FbxNodeAttribute::eMarker):
			str = "eMARKER";
			break;
		case (FbxNodeAttribute::eSkeleton):
			str = "eSKELETON";
			break;
		case (FbxNodeAttribute::eMesh):
			str = "eMESH";
			break;
		case (FbxNodeAttribute::eNurbs):
			str = "eNurbs";
			break;
		case (FbxNodeAttribute::ePatch):
			str = "ePATCH";
			break;
		case (FbxNodeAttribute::eCamera):
			str = "eCAMERA";
			break;
		case (FbxNodeAttribute::eCameraStereo):
			str = "eCAMERA_STEREO";
			break;
		case (FbxNodeAttribute::eCameraSwitcher):
			str = "eCAMERA_SWITCHER";
			break;
		case (FbxNodeAttribute::eLight):
			str = "eLIGHT";
			break;
		case (FbxNodeAttribute::eOpticalReference):
			str = "eOPTICAL_REFERENCE";
			break;
		case (FbxNodeAttribute::eOpticalMarker):
			str = "eOPTICAL_MARKER";
			break;
		case (FbxNodeAttribute::eNurbsCurve):
			str = "eNURBS_CURVE";
			break;
		case (FbxNodeAttribute::eTrimNurbsSurface):
			str = "eTRIM_NURBS_SURFACE";
			break;
		case (FbxNodeAttribute::eBoundary):
			str = "eBOUNDARY";
			break;
		case (FbxNodeAttribute::eNurbsSurface):
			str = "eNURBS_SURFACE";
			break;
		case (FbxNodeAttribute::eShape):
			str = "eSHAPE";
			break;
		case (FbxNodeAttribute::eLODGroup):
			str = "eLODGROUP";
			break;
		case (FbxNodeAttribute::eSubDiv):
			str = "eSUBDIV";
			break;
		case (FbxNodeAttribute::eCachedEffect):
			str = "eCACHED_EFFECT";
			break;
		case (FbxNodeAttribute::eLine):
			str = "eLINE";
			break;
	}

	return str;
}

} // namespace Fbx_utils
