#include "runtime/function/animation/pose.h"

using namespace Pilot;

AnimationPose::AnimationPose() { m_reorder = false; }

AnimationPose::AnimationPose(const AnimationClip& clip, float ratio, const AnimSkelMap& animSkelMap)
{
    m_bone_indexs = animSkelMap.convert;
    m_reorder     = true;
    extractFromClip(m_bone_poses, clip, ratio);
    m_weight.m_blend_weight.resize(m_bone_poses.size());
    for (auto& weight : m_weight.m_blend_weight)
    {
        weight = 1.f;
    }

}
AnimationPose::AnimationPose(const AnimationClip& clip, const BoneBlendWeight& weight, float ratio)
{
    m_weight  = weight;
    m_reorder = false;
    extractFromClip(m_bone_poses, clip, ratio);
}
AnimationPose::AnimationPose(const AnimationClip&   clip,
                             const BoneBlendWeight& weight,
                             float                  ratio,
                             const AnimSkelMap&     animSkelMap)
{
    m_weight      = weight;
    m_bone_indexs = animSkelMap.convert;
    m_reorder     = true;
    extractFromClip(m_bone_poses, clip, ratio);
}

void AnimationPose::extractFromClip(std::vector<Transform>& bones, const AnimationClip& clip, float ratio)
{
    bones.resize(clip.node_count);

    float exact_frame        = ratio * (clip.total_frame - 1);
    int   current_frame_low  = floor(exact_frame); 
    int   current_frame_high = ceil(exact_frame);
    float lerp_ratio         = exact_frame - current_frame_low;
    for (int i = 0; i < clip.node_count; i++)
    {
        const AnimationChannel& channel = clip.node_channels[i];
        bones[i].m_position = Vector3::lerp(
            channel.position_keys[current_frame_low], channel.position_keys[current_frame_high], lerp_ratio);
        bones[i].m_scale    = Vector3::lerp(
            channel.scaling_keys[current_frame_low], channel.scaling_keys[current_frame_high], lerp_ratio);
        bones[i].m_rotation = Quaternion::nLerp(
            lerp_ratio, channel.rotation_keys[current_frame_low], channel.rotation_keys[current_frame_high], true);
    }
}


/**
 * 进行 当前Pose 跟 指定的另一个pose（通过参数传进来）的混合计算。
 */
void AnimationPose::blend(const AnimationPose& pose)
{
    for (int i = 0; i < m_bone_poses.size(); i++)
    {
        auto&       bone_trans_one = m_bone_poses[i];
        const auto& bone_trans_two = pose.m_bone_poses[i];

        float sum_weight = m_weight.m_blend_weight[i] + pose.m_weight.m_blend_weight[i];
        if (sum_weight > 0)
        { //[CR] 若两个 pose 的权重都是0，则它们对骨骼最终的 pose 贡献为0，就没必要做混合计算了。
            
            //[CR] 计算当前 pose 与 指定pose(即输入参数) 的相对权重，并以此来做 transform 的插值。
            float cur_weight = m_weight.m_blend_weight[i] / sum_weight;

            bone_trans_one.m_position = Vector3::lerp(bone_trans_two.m_position, bone_trans_one.m_position, cur_weight);
            bone_trans_one.m_scale    = Vector3::lerp(bone_trans_two.m_scale, bone_trans_one.m_scale, cur_weight);

            //[CR] 为了得到自然的旋转结果，需要将 shortest_path 设为 true。
            bone_trans_one.m_rotation = Quaternion::nLerp(cur_weight, bone_trans_two.m_rotation, bone_trans_one.m_rotation, true);
            
            m_weight.m_blend_weight[i] = sum_weight; //[CR] 由于本 pose 已经应用上了跟 这另一个pose(即输入参数) 混合的结果，故把两者的总权重作为自身的新权重。
        }

        // float sum_weight =
        // if (sum_weight != 0)
        {
            // float cur_weight =
            // m_weight.m_blend_weight[i] =
            // bone_trans_one.m_position  =
            // bone_trans_one.m_scale     =
            // bone_trans_one.m_rotation  =
        }
    }
}



