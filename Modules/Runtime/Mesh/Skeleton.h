#ifndef _EOKAS_MESH_SKELETON_H_
#define _EOKAS_MESH_SKELETON_H_

#include "./Header.h"

namespace eokas
{
    class Skeleton
    {
    public:
        struct Bone
        {
            String name;
            BoneID parent;
        };
        
        struct Transform
        {
            BoneID bone;
            Vector3 localPosition;
            Quaternion localRotation;
            Vector3 localScale;
        };
        
        struct Pose
        {
            std::vector<Transform> transforms;
            
            static Pose add(const Pose& a, const Pose& b);
            static Pose blend(const Pose& a, const Pose& b, float t);
        };
        
        BoneID addBone(const String& name, const BoneID& parent);
        BoneID addBone(const Bone& bone);
        std::vector<BoneID> addBones(const std::vector<Bone>& boneList);
        void setBone(const BoneID& boneId, const Bone& bone);
        u32_t getBoneCount() const;
        BoneID findBone(const String& boneName) const;
        const Bone& getBone(const BoneID& boneId) const;
        
        const Pose& pose() const;
        void skin(RawMesh& mesh);
    
    private:
        std::vector<Bone> mBones;
        Pose mPose;
    };
}

#endif//_EOKAS_MESH_SKELETON_H_