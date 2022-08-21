#include "runtime/function/framework/component/animation/animation_component.h"

#include "runtime/function/animation/animation_system.h"
#include "runtime/function/framework/object/object.h"
#include <runtime/engine.h>

namespace Pilot
{
    void AnimationComponent::postLoadResource(std::weak_ptr<GObject> parent_object)
    {
        m_parent_object = parent_object;

        auto skeleton_res = AnimationManager::tryLoadSkeleton(m_animation_res.m_skeleton_file_path);

        m_skeleton.buildSkeleton(*skeleton_res);
    }

    void AnimationComponent::blend1D(float desired_ratio, BlendSpace1D* blend_state)
    {
        if (blend_state->m_values.size() < 2)
        {
            // no need to interpolate
            return;
        }
        const auto& key_it    = m_signal.find(blend_state->m_key); //[CR] 取出本 BlendSpace1D 所关心的信号的数值
        double      key_value = 0;
        if (key_it != m_signal.end())
        {
            if (key_it->second.is_number())
            {
                key_value = key_it->second.number_value();
            }
        }
        int max_smaller = -1; //[CR] 根据当前的信号值，找到该数值落在哪两个 clip 之间（[ClipA: max_smaller, ClipB: max_smaller+1]），以便后续对这两个 clip 做插值（根据信号值算出来的各自的 weight）
        for (auto value : blend_state->m_values)
        {
            if (value <= key_value)
            {
                max_smaller++;
            }
            else
            {
                break;
            }
        }

        for (auto& weight : blend_state->m_blend_weight)
        { //[CR] 初始化：将所有 clip 的 weight 都先置为 0。由此可知，在 BlendSpace1D 中，真正参与本次 blend 的 clip 不会是全部的 clips——从下面计算 weight 的情况看，至少 1 个、至多 2 个。
            weight = 0;
        }
        if (max_smaller == -1)
        {
            blend_state->m_blend_weight[0] = 1.0f;
        }
        else if (max_smaller == blend_state->m_values.size() - 1)
        {
            blend_state->m_blend_weight[max_smaller] = 1.0f;
        }
        else
        {
            auto l = blend_state->m_values[max_smaller];
            auto r = blend_state->m_values[max_smaller + 1];

            if (l == r)
            {
                // some error
            }

            //[CR] 这条公式跟课件中的 "Calculate Blend Weight" 一致：weight2 = (CurSpeed - Speed1) / (Speed2 - Speed1)
            float weight = (key_value - l) / (r - l);

            blend_state->m_blend_weight[max_smaller + 1] = weight;
            blend_state->m_blend_weight[max_smaller]     = 1 - weight;
        }
        blend(desired_ratio, blend_state);
    }
    void AnimationComponent::tick(float delta_time)
    {
        if ((m_tick_in_editor_mode == false) && g_is_editor_mode)
            return;
        std::string name = m_animation_fsm.getCurrentClipBaseName();
        for (auto blend_state : m_animation_res.m_clips)
        {
            if (blend_state->m_name == name)
            {
                float length        = blend_state->getLength();
                float delta_ratio   = delta_time / length;
                float desired_ratio = delta_ratio + m_ratio;
                if (desired_ratio >= 1.f)
                {
                    desired_ratio = desired_ratio - floor(desired_ratio);
                    updateSignal("clip_finish", true);
                }
                else
                {
                    updateSignal("clip_finish", false);
                }
                bool restart = m_animation_fsm.update(m_signal);
                if (!restart)
                {
                    m_ratio = desired_ratio;
                }
                else
                {
                    m_ratio = 0;
                }
                break;
            }
        }

        name = m_animation_fsm.getCurrentClipBaseName();
        for (auto clip : m_animation_res.m_clips)
        {
            if (clip->m_name == name)
            {
                if (clip.getTypeName() == "BlendSpace1D")
                {
                    auto blend_state_1d_pre = static_cast<BlendSpace1D*>(clip);
                    blend1D(m_ratio, blend_state_1d_pre);
                }
                else if (clip.getTypeName() == "BlendState")
                {
                    auto blend_state = static_cast<BlendState*>(clip);
                    blend(m_ratio, blend_state);
                }
                else if (clip.getTypeName() == "BasicClip")
                {
                    auto basic_clip = static_cast<BasicClip*>(clip);
                    animateBasicClip(m_ratio, basic_clip);
                }
                break;
            }
        }
    }

    const AnimationResult& AnimationComponent::getResult() const { return m_animation_result; }

    void AnimationComponent::animateBasicClip(float desired_ratio, BasicClip* basic_clip)
    {
        auto                       clip_data = AnimationManager::getClipData(*basic_clip);

            AnimationPose pose(clip_data.m_clip,
                               desired_ratio,
                               clip_data.m_anim_skel_map);
            m_skeleton.resetSkeleton();
            m_skeleton.applyAdditivePose(pose);
            m_skeleton.extractPose(pose);
        

        m_skeleton.applyPose(pose);
        m_animation_result = m_skeleton.outputAnimationResult();
    }
    void AnimationComponent::blend(float desired_ratio, BlendState* blend_state)
    {

        //[CR] Q: 为何需要都初始化为 desired_ratio？
        //     A: 因为 BlendState 中所有的 clips 都要受这个 ratio 影响——每个 clip 都根据 ratio 来算出自己的 ”目标 frame“（一般是通过插值得到，详见代码：Skeleton::applyAnimation）
        for (auto& ratio : blend_state->m_blend_ratio)
        {
            ratio = desired_ratio;
        }
        auto                       blendStateData = AnimationManager::getBlendStateWithClipData(*blend_state);
        std::vector<AnimationPose> poses;
        for (int i = 0; i < blendStateData.m_clip_count; i++)
        {
            //[CR] 根据指定的 clip、指定的 weight、ratio、skel_map，得到该 clip 在该时刻（ratio）和权重（weight）下的 pose 数据（各 bones 相对于 bind pose 的 transform (local space?)）
            AnimationPose pose(blendStateData.m_blend_clip[i],
                               blendStateData.m_blend_weight[i],
                               blendStateData.m_blend_ratio[i],
                               blendStateData.m_blend_anim_skel_map[i]);

            //[CR] 重置为 bind pose (A Pose or T Pose) 时的状态（各 bones 的 transform 都重置了）
            m_skeleton.resetSkeleton();

            //[CR] 将当前 pose 应用到 bind pose，以将骨骼变成了当前 pose 所表示的样子——也就是各个 bones 变成了该 pose 下的 transform；
            //  并且，各个 bones 自己也计算好了 model space 下自己的 transform ——即 skinning matrix 的左边那个矩阵 Mjm(t)
            m_skeleton.applyAdditivePose(pose);

            //[CR] 把骨骼的当前 pose 数据转存到 'pose' 这个变量、并放入 poses 中，以便接下来对本 BlendState 中所有的 pose 做 blending
            m_skeleton.extractPose(pose);
            poses.push_back(pose);
        }
        for (int i = 1; i < blendStateData.m_clip_count; i++)
        {
            for (auto& pose : poses[i].m_weight.m_blend_weight)
            {
                pose = blend_state->m_blend_weight[i];
            }
            poses[0].blend(poses[i]);
        }

        m_skeleton.applyPose(poses[0]);
        m_animation_result = m_skeleton.outputAnimationResult();
    }
} // namespace Pilot
