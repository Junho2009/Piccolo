#pragma once

#include "runtime/core/base/macro.h"
#include "runtime/core/meta/serializer/serializer.h"

#include <filesystem>
#include <fstream>
#include <functional>
#include <sstream>
#include <string>

#include "_generated/serializer/all_serializer.h"

namespace Piccolo
{
    class AssetManager
    {
    public:
        /** [CR]
         * 加载指定的资产
         * @asset_url 资产路径。
         *   从调试情况看，形如这样的路径："asset/objects/character/player/components/animation/data/W2_CrouchWalk_Aim_F_Loop_IP.animation_clip.json"。
         *     p.s.: 这个资产路径，是存储在 AnimationComponent 的序列化数据 "AnimationComponentRes" 之中的。
         *   经过 getFullPath 的转换之后，路径变成
         */
        template<typename AssetType>
        bool loadAsset(const std::string& asset_url, AssetType& out_asset) const
        {
            // read json file to string
            std::filesystem::path asset_path = getFullPath(asset_url);
            std::ifstream asset_json_file(asset_path);
            if (!asset_json_file)
            {
                LOG_ERROR("open file: {} failed!", asset_path.generic_string());
                return false;
            }

            std::stringstream buffer;
            buffer << asset_json_file.rdbuf();
            std::string asset_json_text(buffer.str());

            // parse to json object and read to runtime res object
            std::string error;
            auto&&      asset_json = PJson::parse(asset_json_text, error);
            if (!error.empty())
            {
                LOG_ERROR("parse json file {} failed!", asset_url);
                return false;
            }

            /** [CR]
             * 这个 read 函数有多种实现。其中比较特别的是，"PiccoloParser" 为各种反射类型生成了特化版本的 read 函数——根据该类型中的各种反射成员。
             */
            PSerializer::read(asset_json, out_asset);
            return true;
        }

        template<typename AssetType>
        bool saveAsset(const AssetType& out_asset, const std::string& asset_url) const
        {
            std::ofstream asset_json_file(getFullPath(asset_url));
            if (!asset_json_file)
            {
                LOG_ERROR("open file {} failed!", asset_url);
                return false;
            }

            // write to json object and dump to string
            auto&&        asset_json      = PSerializer::write(out_asset);
            std::string&& asset_json_text = asset_json.dump();

            // write to file
            asset_json_file << asset_json_text;
            asset_json_file.flush();

            return true;
        }

        std::filesystem::path getFullPath(const std::string& relative_path) const;

    };
} // namespace Piccolo
