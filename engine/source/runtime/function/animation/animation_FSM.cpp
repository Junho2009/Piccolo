#include "runtime/function/animation/animation_FSM.h"
#include <iostream>

#include "core/base/macro.h"

namespace Pilot
{
    std::string AnimationFSM::toStateName(States state)
    {
        switch (state)
        {
        case States::_idle:
            return "_idle";

        case States::_walk_start:
            return "_walk_start";

        case States::_walk_run:
            return "_walk_run";

        case States::_walk_stop:
            return "_walk_stop";

        case States::_jump_start_from_idle:
            return "_jump_start_from_idle";

        case States::_jump_loop_from_idle:
            return "_jump_loop_from_idle";

        case States::_jump_end_from_idle:
            return "_jump_end_from_idle";

        case States::_jump_start_from_walk_run:
            return "_jump_start_from_walk_run";

        case States::_jump_loop_from_walk_run:
            return "_jump_loop_from_walk_run";

        case States::_jump_end_from_walk_run:
            return "_jump_end_from_walk_run";

        default:
            return "[???]";
        }
    }

    AnimationFSM::AnimationFSM() {}
    float tryGetFloat(const json11::Json::object& json, const std::string& key, float default_value)
    {
        auto found_iter = json.find(key);
        if (found_iter != json.end() && found_iter->second.is_number())
        {
            return found_iter->second.number_value();
        }
        return default_value;
    }
    bool tryGetBool(const json11::Json::object& json, const std::string& key, float default_value)
    {
        auto found_iter = json.find(key);
        if (found_iter != json.end() && found_iter->second.is_bool())
        {
            return found_iter->second.bool_value();
        }
        return default_value;
    }
    bool AnimationFSM::update(const json11::Json::object& signals)
    {
        States last_state     = m_state;
        bool   is_clip_finish = tryGetBool(signals, "clip_finish", false);
        bool   is_jumping     = tryGetBool(signals, "jumping", false);
        float  speed          = tryGetFloat(signals, "speed", 0);
        bool   is_moving      = speed > 0.01f;
        bool   start_walk_end = (States::_walk_run == m_state && !is_moving);//false;

        switch (m_state)
        {
            case States::_idle:
                if (is_jumping)
                {
                    m_state = States::_jump_start_from_idle;
                }
                else if (is_moving)
                {
                    m_state = States::_walk_start;
                }
                break;

            
            case States::_walk_start:
                if (is_jumping)
                {
                    //NOTE: 相比 作业pdf 中多出来的转换条件，作用是实现从 _walk_start 到 _jump_start_from_walk_run 的直接跳转，
                    //   避免在开始 walk 时即按下跳跃键、造成 “人的高度已经往上抬了、但动作还是 walk_start” 这样的奇怪表现。
                    m_state = States::_jump_start_from_walk_run;
                }
                else if (!is_moving)
                {
                    //NOTE: 同理，在 _walk_start 的过程中，若已经停止移动，则切回到 _idle 状态。
                    m_state = States::_idle;
                }
                else if (is_clip_finish)
                {
                    m_state = States::_walk_run;
                }
                break;

            
            case States::_walk_run:
                if (is_jumping)
                {
                    m_state = States::_jump_start_from_walk_run;
                }
                else if (start_walk_end && is_clip_finish)
                {
                    m_state = States::_walk_stop;
                }
                else if (!is_moving)
                {
                    m_state = States::_idle;
                }
                break;

            
            case States::_walk_stop:
                if (!is_moving && is_clip_finish)
                {
                    m_state = States::_idle;
                }
                break;

            
            case States::_jump_start_from_idle:
                if (is_clip_finish)
                {
                    m_state = States::_jump_loop_from_idle;
                }
                break;

            
            case States::_jump_loop_from_idle:
                if (!is_jumping)
                {
                    m_state = States::_jump_end_from_idle;
                }
                break;

            
            case States::_jump_end_from_idle:
                if (is_clip_finish)
                {
                    m_state = States::_idle;
                }
                break;

            
            case States::_jump_start_from_walk_run:
                if (is_clip_finish)
                {
                    m_state = States::_jump_loop_from_walk_run;
                }
                break;

            
            case States::_jump_loop_from_walk_run:
                if (!is_jumping)
                {
                    m_state = States::_jump_end_from_walk_run;
                }
                break;

            
            case States::_jump_end_from_walk_run:
                if (is_clip_finish)
                {
                    m_state = States::_walk_run;
                }
                break;

            
            default:
                break;
        }

        LOG_INFO("FSM : speed: {}, last: {}, cur: {}", speed, toStateName(last_state), toStateName(m_state));
        
        return last_state != m_state;
    }

    std::string AnimationFSM::getCurrentClipBaseName() const
    {
        switch (m_state)
        {
            case States::_idle:
                return "idle_walk_run";
            case States::_walk_start:
                return "walk_start";
            case States::_walk_run:
                return "idle_walk_run";
            case States::_walk_stop:
                return "walk_stop";
            case States::_jump_start_from_walk_run:
            case States::_jump_start_from_idle:
                return "jump_start";
            case States::_jump_loop_from_walk_run:
            case States::_jump_loop_from_idle:
                return "jump_loop";
            case States::_jump_end_from_walk_run:
            case States::_jump_end_from_idle:
                return "jump_stop";
            default:
                return "idle_walk_run";
        }
    }
}

