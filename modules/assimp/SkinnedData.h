#ifndef SKINNEDDATA_H_
#define SKINNEDDATA_H_

#include "MathHelper.h"
#include "core/math/plane.h"
#include "core/math/transform.h"
#include "core/math/vector3.h"
#include <map>
#include <vector>
///<summary>
/// A Keyframe defines the bone transformation at an instant in time.
///</summary>
struct Keyframe {
	Keyframe();
	~Keyframe();

	float TimePos;
	Vector3 Translation;
	Vector3 Scale;
	Quat RotationQuat;

	bool operator==(const Keyframe &key) {
		if (Translation.x != key.Translation.x || Translation.y != key.Translation.y || Translation.z != key.Translation.z)
			return false;

		if (Scale.x != key.Scale.x || Scale.y != key.Scale.y || Scale.z != key.Scale.z)
			return false;

		if (RotationQuat.x != key.RotationQuat.x || RotationQuat.y != key.RotationQuat.y || RotationQuat.z != key.RotationQuat.z || RotationQuat.w != key.RotationQuat.w)
			return false;

		return true;
	}
};

///<summary>
/// A BoneAnimation is defined by a list of keyframes.  For time
/// values inbetween two keyframes, we interpolate between the
/// two nearest keyframes that bound the time.
///
/// We assume an animation always has two keyframes.
///</summary>
struct BoneAnimation {
	float GetStartTime() const;
	float GetEndTime() const;

	void Interpolate(float t, Transform &M) const;

	std::vector<Keyframe> Keyframes;
};

///<summary>
/// Examples of AnimationClips are "Walk", "Run", "Attack", "Defend".
/// An AnimationClip requires a BoneAnimation for every bone to form
/// the animation clip.
///</summary>
struct AnimationClip {
	float GetClipStartTime() const;
	float GetClipEndTime() const;

	void Interpolate(float t, std::vector<Transform> &boneTransforms) const;

	std::vector<BoneAnimation> BoneAnimations;
};

class SkinnedData {
public:
	uint32_t BoneCount() const;

	float GetClipStartTime(const String &clipName) const;
	float GetClipEndTime(const String &clipName) const;
	String GetAnimationName(int num) const;
	const std::vector<int> &GetBoneHierarchy() const;
	const std::vector<Transform> &GetBoneOffsets() const;
	AnimationClip GetAnimation(String clipName) const;
	const std::vector<int> &GetSubmeshOffset() const;
	Transform getBoneOffsets(int num) const;
	const std::vector<String> &GetBoneName() const;

	void Set(
			std::vector<int> &boneHierarchy,
			std::vector<Transform> &boneOffsets,
			std::map<String, AnimationClip> *animations = nullptr);
	void SetAnimation(AnimationClip inAnimation, String inClipName);
	void SetAnimationName(const String &clipName);
	void SetBoneName(String boneName);
	void SetSubmeshOffset(int num);

	void clear();

	// In a real project, you'd want to cache the result if there was a chance
	// that you were calling this several times with the same clipName at
	// the same timePos.
	void GetFinalTransforms(const String &clipName, float timePos,
			std::vector<Transform> &finalTransforms) const;

private:
	std::vector<String> mBoneName;

	// Gives parentIndex of ith bone.
	std::vector<int> mBoneHierarchy;

	std::vector<Transform> mBoneOffsets;

	std::vector<String> mAnimationName;
	std::map<String, AnimationClip> mAnimations;

	std::vector<int> mSubmeshOffset;
};
#endif