#pragma once
#include "json11.hpp"

/** [CR]
 * 通过在 Core 层来为第三方库起别名，然后在引擎内使用该别名，以便更换具体的第三方库。
 *
 * p.s.: C++11 开始，可以使用 using 来指定别名，作用跟 typedef 一样。
 *   资料：C++中using的三种用法 - https://zhuanlan.zhihu.com/p/156155959
 */
using PJson = json11::Json;