#include "./Skeleton.h"
#include "./RawMesh.h"

namespace eokas
{
    Skeleton::Pose Skeleton::Pose::add(const Pose& a, const Pose& b)
    {
        Pose result;
        if (a.transforms.size() != b.transforms.size())
        {
            return result;
        }
        
        result.transforms.resize(a.transforms.size());
        
        for (size_t i = 0; i < a.transforms.size(); ++i)
        {
            const auto& transformA = a.transforms[i];
            const auto& transformB = b.transforms[i];
            if (transformA.bone != transformB.bone)
            {
                continue;
            }
            
            Vector3 combinedPos = transformA.localPosition + transformB.localPosition;
            Quaternion combinedRot = transformA.localRotation * transformB.localRotation;
            Vector3 combinedScale = transformA.localScale * transformB.localScale;
            
            result.transforms[i] = {transformA.bone, combinedPos, combinedRot, combinedScale};
        }
        
        return result;
    }
    
    Skeleton::Pose Skeleton::Pose::blend(const Pose& a, const Pose& b, float t)
    {
        Pose result;
        if (a.transforms.size() != b.transforms.size())
        {
            return result;
        }
        
        result.transforms.resize(a.transforms.size());
        
        t = std::clamp(t, 0.0f, 1.0f);
        for (size_t i = 0; i < a.transforms.size(); ++i)
        {
            const auto& transformA = a.transforms[i];
            const auto& transformB = b.transforms[i];
            if (transformA.bone != transformB.bone)
            {
                continue;
            }
            
            Vector3 blendedPos = Math::lerp(transformA.localPosition, transformB.localPosition, t);
            Quaternion blendedRot = Math::slerp(transformA.localRotation, transformB.localRotation, t);
            Vector3 blendedScale = Math::lerp(transformA.localScale, transformB.localScale, t);
            
            result.transforms[i] = {transformA.bone, blendedPos, blendedRot, blendedScale};
        }
        
        return result;
    }
    
    BoneID Skeleton::addBone(const String& name, const BoneID& parent)
    {
        Bone& bone = this->mBones.emplace_back();
        bone.name = name;
        bone.parent = parent;
        return this->mBones.size() - 1;
    }
    
    BoneID Skeleton::addBone(const Bone& bone)
    {
        this->mBones.push_back(bone);
        return this->mBones.size() - 1;
    }
    
    std::vector<BoneID> Skeleton::addBones(const std::vector<Bone>& boneList)
    {
        std::vector<BoneID> result;
        for(const auto& bone : boneList)
        {
            BoneID boneId = this->addBone(bone);
            result.push_back(boneId);
        }
        return result;
    }
    
    void Skeleton::setBone(const BoneID& boneId, const Bone& bone)
    {
        if(boneId < this->mBones.size())
        {
            this->mBones[boneId] = bone;
        }
    }
    
    u32_t Skeleton::getBoneCount() const
    {
        return static_cast<u32_t>(this->mBones.size());
    }
    
    BoneID Skeleton::findBone(const String& boneName) const
    {
        for(size_t index = 0; index < this->mBones.size(); index++)
        {
            const Bone& bone = this->mBones.at(index);
            if(bone.name == boneName)
                return index;
        }
        return {};
    }
    
    const Skeleton::Bone& Skeleton::getBone(const BoneID& boneId) const
    {
        return this->mBones.at(boneId);
    }
    
    const Skeleton::Pose& Skeleton::pose() const
    {
        return mPose;
    }
    
    void Skeleton::skin(RawMesh& mesh)
    {
    
    }
}
