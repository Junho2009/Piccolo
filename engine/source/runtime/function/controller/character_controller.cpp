#include "runtime/function/controller/character_controller.h"

#include "runtime/core/base/macro.h"

#include "runtime/function/framework/component/motor/motor_component.h"
#include "runtime/function/framework/world/world_manager.h"
#include "runtime/function/global/global_context.h"
#include "runtime/function/physics/physics_scene.h"

namespace Pilot
{
    CharacterController::CharacterController(const Capsule& capsule) : m_capsule(capsule)
    {
        m_rigidbody_shape                                    = RigidBodyShape();
        m_rigidbody_shape.m_geometry                         = PILOT_REFLECTION_NEW(Capsule);
        *static_cast<Capsule*>(m_rigidbody_shape.m_geometry) = m_capsule;

        m_rigidbody_shape.m_type = RigidBodyShapeType::capsule;

        Quaternion orientation;
        orientation.fromAngleAxis(Radian(Degree(90.f)), Vector3::UNIT_X);

        m_rigidbody_shape.m_local_transform =
            Transform(
                Vector3(0, 0, capsule.m_half_height + capsule.m_radius),
                orientation,
                Vector3::UNIT_SCALE);
    }


    //[CR] displacement: 在不考虑物理碰撞的情况下、计算出来的期望位移值
    Vector3 CharacterController::move(const Vector3& current_position, const Vector3& displacement)
    {
        std::shared_ptr<PhysicsScene> physics_scene =
            g_runtime_global_context.m_world_manager->getCurrentActivePhysicsScene().lock();
        ASSERT(physics_scene);

        std::vector<PhysicsHitInfo> hits;

        Transform world_transform = Transform(
            current_position + 0.1f * Vector3::UNIT_Z, //[CR] 当前理解：为了避免角色在更新位置的过程中、因为一些误差原因而陷入到地面中，在进行物理碰撞检测之前，先给一个垂直向上的偏移
            Quaternion::IDENTITY,
            Vector3::UNIT_SCALE);

        Vector3 vertical_displacement   = displacement.z * Vector3::UNIT_Z; //[CR] 得到 垂直 方向的位移向量
        Vector3 horizontal_displacement = Vector3(displacement.x, displacement.y, 0.f); //[CR] 得到 水平 方向的位移向量

        Vector3 vertical_direction   = vertical_displacement.normalisedCopy(); //[CR] 得到 垂直 方向向量
        Vector3 horizontal_direction = horizontal_displacement.normalisedCopy(); //[CR] 得到 水平 方向向量

        Vector3 final_position = current_position;

        m_is_touch_ground = physics_scene->sweep(
            m_rigidbody_shape,
            world_transform.getMatrix(),
            Vector3::NEGATIVE_UNIT_Z, //[CR] -Z 是垂直于地面 向下 的方向（+Z 是垂直向上）
            0.105f,
            hits);

        hits.clear(); //[CR] 为什么要清掉呢，不需要根据 hits 结果来决定 "角色的脚应该在哪个高度"？
        
        world_transform.m_position -= 0.1f * Vector3::UNIT_Z; //[CR] 物理检测计算完毕，恢复之前所做的偏移

        // vertical pass
        if (physics_scene->sweep(
            m_rigidbody_shape,
            world_transform.getMatrix(),
            vertical_direction,
            vertical_displacement.length(),
            hits))
        {
            final_position += hits[0].hit_distance * vertical_direction;
        }
        else
        {
            final_position += vertical_displacement;
        }

        hits.clear();

        // horizontal pass
        if (physics_scene->sweep(
            m_rigidbody_shape,
            world_transform.getMatrix(),
            horizontal_direction,
            horizontal_displacement.length(),
            hits
            ))
        {
            final_position += hits[0].hit_distance * horizontal_direction;
        }
        else
        {
            final_position += horizontal_displacement;
        }

        return final_position;
    }

} // namespace Pilot
