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

        hits.clear();

        world_transform = Transform(
            final_position,
            Quaternion::IDENTITY,
            Vector3::UNIT_SCALE);

        // vertical pass
        if (physics_scene->sweep(
            m_rigidbody_shape,
            world_transform.getMatrix(),
            vertical_direction,
            vertical_displacement.length(),
            hits))
        {
            //TODO: 为了让角色在下落时、碰到墙壁还能落下，这里就需要区分【下落到地面】和【下落到墙壁】两种情况

            final_position += hits[0].hit_distance * vertical_direction;
            
            /*PhysicsHitInfo* vertical_hit = nullptr;
            for (auto& hit : hits)
            {
                const Vector3 hit_normal = hit.hit_normal.normalisedCopy();
                const float dot = hit_normal.dotProduct(vertical_direction);
                //LOG_INFO("dot: {}", dot)
                if (dot >= 0.9) // 过滤掉跟垂直方向差异较大的 hit
                {
                    vertical_hit = &hit;
                    break;
                }
                else
                {
                    int iii = 0;
                    ++iii;
                    LOG_INFO("vertical_hit++3 ")
                }
            }

            if (nullptr != vertical_hit)
            {
                const Vector3 n = vertical_hit->hit_normal;
                LOG_INFO("vertical_hit--------1 hit dir: ({}, {}, {})", n.x, n.y, n.z)
                final_position += vertical_hit->hit_distance * vertical_direction;
            }
            else
            {
                const Vector3 ver_disp = vertical_displacement;
                LOG_INFO("vertical_hit====2 null. ver_disp: ({}, {}, {})", ver_disp.x, ver_disp.y, ver_disp.z)

                // Hack... sweep 测试通过，说明垂直方向是有发生位移的。既然没找到垂直方向的 hit，就直接处理下落？
                //   但是，这样可能会卡在空中，因为加上 vertical_displacement 之后，并不能确保角色可以回到地面。
                //   然而，目前还没找到能让角色移到地面的信息（和方法）。
                final_position += vertical_displacement;
            }*/
        }
        else
        {
            final_position += vertical_displacement;
        }

        hits.clear();

        return final_position;
    }

} // namespace Pilot
