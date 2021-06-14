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
	float t = 90000.0f;
	Map<String, BoneAnimation>::Element *node = BoneAnimationsForName.front();
	while (node) {
		t = MIN(t, node->value().GetStartTime());

		node = node->next();
	}

	return t;
}
float AnimationClip::GetClipEndTime() const {
	// Find largest end time over all bones in this clip.
	float t = 0.0f;
	Map<String, BoneAnimation>::Element *node = BoneAnimationsForName.front();
	while (node) {
		t = MAX(t, node->value().GetEndTime());
		node = node->next();
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

Transform SkinnedData::getBoneOffsets(int num) const {
	return mBoneOffsets.at(num);
}
