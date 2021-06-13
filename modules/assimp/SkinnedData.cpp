#include "SkinnedData.h"

Keyframe::Keyframe() :
		TimePos(0.0f),
		Translation(0.0f, 0.0f, 0.0f),
		Scale(1.0f, 1.0f, 1.0f),
		RotationQuat(0.0f, 0.0f, 0.0f, 1.0f) {
}

Keyframe::~Keyframe() {
}

float BoneAnimation::GetStartTime() const {
	// Keyframes are sorted by time, so first keyframe gives start time.
	return Keyframes.front().TimePos;
}
float BoneAnimation::GetEndTime() const {
	// Keyframes are sorted by time, so last keyframe gives end time.
	if (Keyframes.size() > 0)
		return Keyframes.back().TimePos;

	return 0.0f;
}
float AnimationClip::GetClipStartTime() const {
	// Find smallest start time over all bones in this clip.
	float t = MathHelper::Infinity;
	for (uint32_t i = 0; i < BoneAnimations.size(); ++i) {
		t = MathHelper::Min(t, BoneAnimations[i].GetStartTime());
	}

	return t;
}
float AnimationClip::GetClipEndTime() const {
	// Find largest end time over all bones in this clip.
	float t = 0.0f;
	for (uint32_t i = 0; i < BoneAnimations.size(); ++i) {
		t = MathHelper::Max(t, BoneAnimations[i].GetEndTime());
	}

	return t;
}
float SkinnedData::GetClipStartTime(const String &clipName) const {
	auto clip = mAnimations.find(clipName);
	return clip->second.GetClipStartTime();
}
float SkinnedData::GetClipEndTime(const String &clipName) const {
	auto clip = mAnimations.find(clipName);
	return clip->second.GetClipEndTime();
}
String SkinnedData::GetAnimationName(int num) const {
	return mAnimationName.at(num);
}
uint32_t SkinnedData::BoneCount() const {
	return (uint32_t)mBoneHierarchy.size();
}
const std::vector<String> &SkinnedData::GetBoneName() const {
	return mBoneName;
}
const std::vector<int> &SkinnedData::GetBoneHierarchy() const {
	return mBoneHierarchy;
}
const std::vector<Transform> &SkinnedData::GetBoneOffsets() const {
	return mBoneOffsets;
}
AnimationClip SkinnedData::GetAnimation(String clipName) const {
	return mAnimations.find(clipName)->second;
}
const std::vector<int> &SkinnedData::GetSubmeshOffset() const {
	return mSubmeshOffset;
}

void BoneAnimation::Interpolate(float t, Transform &M) const {
	if (t <= Keyframes.front().TimePos) {
		Vector3 S = Vector3(Keyframes.front().Scale);
		Vector3 P = Vector3(Keyframes.front().Translation);
		Quat Q = (Keyframes.front().RotationQuat);

		M.origin = P;
		M.basis.set_quat_scale(Q, S);
	} else if (t >= Keyframes.back().TimePos) {
		Vector3 S = (Keyframes.back().Scale);
		Vector3 P = (Keyframes.back().Translation);
		Quat Q = (Keyframes.back().RotationQuat);

		M.origin = P;
		M.basis.set_quat_scale(Q, S);
	} else {
		for (uint32_t i = 0; i < Keyframes.size() - 1; ++i) {
			if (t >= Keyframes[i].TimePos && t <= Keyframes[i + 1].TimePos) {
				float lerpPercent = (t - Keyframes[i].TimePos) / (Keyframes[i + 1].TimePos - Keyframes[i].TimePos);

				Vector3 s0 = (Keyframes[i].Scale);
				Vector3 s1 = (Keyframes[i + 1].Scale);

				Vector3 p0 = (Keyframes[i].Translation);
				Vector3 p1 = (Keyframes[i + 1].Translation);

				Quat q0 = (Keyframes[i].RotationQuat);
				Quat q1 = (Keyframes[i + 1].RotationQuat);

				Vector3 S = s0.slerp(s1, lerpPercent);
				Vector3 P = p0.slerp(p1, lerpPercent);
				Quat Q = q0.slerp(q1, lerpPercent);

				M.origin = P;
				M.basis.set_quat_scale(Q, S);

				break;
			}
		}
	}
}
void AnimationClip::Interpolate(float t, std::vector<Transform> &boneTransforms) const {
	for (uint32_t i = 0; i < BoneAnimations.size(); ++i) {
		BoneAnimations[i].Interpolate(t, boneTransforms[i]);
	}
}

void SkinnedData::SetAnimation(AnimationClip inAnimation, String ClipName) {
	mAnimations[ClipName] = inAnimation;
}
void SkinnedData::SetAnimationName(const String &clipName) {
	mAnimationName.push_back(clipName);
}
void SkinnedData::SetBoneName(String boneName) {
	mBoneName.push_back(boneName);
}
void SkinnedData::SetSubmeshOffset(int num) {
	mSubmeshOffset.push_back(num);
}

void SkinnedData::clear() {
	mBoneName.clear();
	mBoneHierarchy.clear();
	mBoneOffsets.clear();
	mAnimationName.clear();
	mAnimations.clear();
	mSubmeshOffset.clear();
}
//
//void printMatrix(const std::wstring& Name, const float& i, const DirectX::XMMATRIX &M)
//{
//	std::wstring text = Name + std::to_wstring(i) + L"\n";
//	::OutputDebugString(text.c_str());
//
//	for (int j = 0; j < 4; ++j)
//	{
//		for (int k = 0; k < 4; ++k)
//		{
//			std::wstring text =
//				std::to_wstring(M.r[j].m128_f32[k]) + L" ";
//
//			::OutputDebugString(text.c_str());
//		}
//		std::wstring text = L"\n";
//		::OutputDebugString(text.c_str());
//
//	}
//}

void SkinnedData::GetFinalTransforms(const String &clipName, float timePos, std::vector<Transform> &finalTransforms) const {
	uint32_t numBones = (uint32_t)mBoneOffsets.size();

	std::vector<Transform> toParentTransforms(numBones);

	// Interpolate all the bones of this clip at the given time instance.
	auto clip = mAnimations.find(clipName);
	clip->second.Interpolate(timePos, toParentTransforms);

	//
	// Traverse the hierarchy and transform all the bones to the root space.
	//
	std::vector<Transform> toRootTransforms(numBones);

	// The root bone has index 0.  The root bone has no parent, so its toRootTransform
	// is just its local bone transform.
	toRootTransforms[0] = toParentTransforms[0];

	// Now find the toRootTransform of the children.
	for (uint32_t i = 1; i < numBones; ++i) {
		Transform toParent = (toParentTransforms[i]);

		int parentIndex = mBoneHierarchy[i];
		Transform parentToRoot = (toRootTransforms[parentIndex]);

		toRootTransforms[i] = (toParent * parentToRoot);
	}

	// Premultiply by the bone offset transform to get the final transform.
	for (uint32_t i = 0; i < numBones; ++i) {
		Transform offset = (mBoneOffsets[i]);
		Transform toRoot = (toParentTransforms[i]);
		Transform finalTransform = (offset * toRoot);

		//printMatrix(L"Offset", i, offset);
		//printMatrix(L"toRoot", i, toRoot);
		//if(i == 15)
		//printMatrix(L"final", timePos, finalTransform);

		finalTransforms[i] = (finalTransform);
	}
}

Transform SkinnedData::getBoneOffsets(int num) const {
	return mBoneOffsets.at(num);
}
