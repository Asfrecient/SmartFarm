//
// Created by keysk on 2025/11/24.
//

#ifndef SMARTFARM_SCREEN_H
#define SMARTFARM_SCREEN_H

/**
 * @file screen.h
 * @brief 屏幕页面和阈值编辑状态管理模块
 * 
 * 本模块管理OLED屏幕的页面切换和阈值编辑功能：
 * - 页面索引：控制显示哪个页面（首页/阈值设置页）
 * - 阈值编辑索引：在阈值设置页中选择要编辑的阈值项
 * - 阈值编辑状态：区分浏览模式和编辑模式
 * 
 * 这些状态由InputTask根据用户输入更新，由ScreenTask读取用于显示
 */

/**
 * @brief 屏幕页面索引枚举
 * 
 * 定义系统支持的页面类型，用于页面切换
 */
typedef enum {
  PAGE_HOME = 0,  // 首页：显示当前环境状态数据
  PAGE_RANGE = 1, // 阈值设置页：显示和编辑报警阈值
  PAGE_End,       // 结束标志：用于循环切换
} ScreenPage;

extern ScreenPage pageIndex; // 当前显示的页面索引

/**
 * @brief 切换到下一个页面
 * 
 * 循环切换页面：首页 -> 阈值设置页 -> 首页
 * 由InputTask在用户按下KEY1时调用
 */
void ScreenPage_NextPage();

/**
 * @brief 阈值编辑索引枚举
 * 
 * 定义所有可编辑的阈值项，用于在阈值设置页中选择要编辑的参数
 * 按显示顺序排列，便于用户通过旋钮切换
 */
typedef enum {
  RANGE_EDIT_TEMPERATURE_MIN = 0,      // 最小温度报警阈值
  RANGE_EDIT_TEMPERATURE_MAX = 1,      // 最大温度报警阈值
  RANGE_EDIT_HUMIDITY_MIN = 2,         // 最小湿度报警阈值
  RANGE_EDIT_HUMIDITY_MAX = 3,         // 最大湿度报警阈值
  RANGE_EDIT_LIGHT_INTENSITY_MIN = 4,  // 最小光照强度报警阈值
  RANGE_EDIT_LIGHT_INTENSITY_MAX = 5,  // 最大光照强度报警阈值
  RANGE_EDIT_SOIL_MOISTURE_MIN = 6,    // 最小土壤湿度报警阈值
  RANGE_EDIT_SOIL_MOISTURE_MAX = 7,    // 最大土壤湿度报警阈值
  RANGE_EDIT_RAIN_GAUGE_MAX = 8,       // 最大降雨量报警阈值
  RANGE_EDIT_END,                      // 结束标志：用于循环切换
} RangeEditIndex;

extern RangeEditIndex rangeEditIndex; // 当前选中的阈值编辑索引

/**
 * @brief 切换到下一个阈值编辑项
 * 
 * 循环切换：从当前项到下一项，到达末尾时回到第一项
 * 在阈值设置页的浏览模式下，由InputTask根据旋钮旋转调用
 */
void RangeEditIndex_Next();

/**
 * @brief 切换到上一个阈值编辑项
 * 
 * 循环切换：从当前项到上一项，在第一项时跳到最后一项
 * 在阈值设置页的浏览模式下，由InputTask根据旋钮旋转调用
 */
void RangeEditIndex_Prev();

/**
 * @brief 阈值编辑状态枚举
 * 
 * 区分两种模式：
 * - 正常状态：浏览模式，旋钮用于切换选中的阈值项
 * - 编辑状态：编辑模式，旋钮用于修改当前选中项的值
 */
typedef enum {
  RANGE_EDIT_STATE_NORMAL = 0,  // 正常状态：浏览模式
  RANGE_EDIT_STATE_EDITING = 1,  // 编辑状态：编辑模式
} RangeEditState;

extern RangeEditState rangeEditState; // 当前阈值编辑状态

/**
 * @brief 进入阈值编辑状态
 * 
 * 切换到编辑模式，此时旋钮可以修改当前选中阈值项的值
 */
void RangeEditState_EnterEditing();

/**
 * @brief 退出阈值编辑状态
 * 
 * 切换回浏览模式，此时旋钮用于切换选中的阈值项
 */
void RangeEditState_QuitEditing();

/**
 * @brief 切换阈值编辑状态
 * 
 * 在编辑模式和浏览模式之间切换
 * 由InputTask在用户按下KEY3时调用
 */
void RangeEditState_Toggle();

#endif //SMARTFARM_SCREEN_H