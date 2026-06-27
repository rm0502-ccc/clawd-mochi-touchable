# Clawd Mochi Touchable 🦀✋

基于 [@yousifamanuel](https://github.com/yousifamanuel) 的 [Clawd Mochi](https://github.com/yousifamanuel/clawd-mochi) 修改，新增触摸交互、时间感知行为和 15 种动画表情。

原版 Clawd Mochi 是一个由 ESP32-C3 驱动的桌面小伙伴，搭配 1.54 寸 TFT 彩屏。本 fork 保留了原版的全部硬件和接线方案，仅大幅扩展了固件，把它从一个遥控显示器变成了有自主行为的桌面宠物。

> ⚠️ 本项目为独立同人作品，与 Anthropic 无关联、无赞助、无背书。"Claude"和"Clawd"是 Anthropic 的商标。

---

## 和原版的区别

### 15 种新表情（4 → 19 种）

| 表情 | 说明 |
|---|---|
| 😠 生气 | 皱眉 + 抖动 |
| 😢 伤心 | 下垂眼 + 眼泪掉落动画 |
| 😪 犯困 | 眼皮逐渐合上 |
| 😴 睡觉 | 闭眼 + z Z Z 逐级浮起 |
| 🤔 思考 | 向上看 + 思考气泡逐个出现并飘移 |
| 😊 开心 | 倒V眼 + 弹跳 |
| 😒 不爽 | 一只眼斜视 + 侧瞥 |
| 😘 亲亲 | 眨眼 + 爱心生长、脉动、爆炸成粒子 |
| 😉 眨眼 | 快速眨眼节奏 |
| 🫧 冒泡 | 正常眼 + 气泡上浮 |
| 😑 无聊 | 半闭眼 + 缓慢左右飘移带倾斜 |
| 😕 困惑 | 不对称眼睛 |
| 😵 晕 | 螺旋眼（触摸过载触发） |
| 💀 死机 | X 形眼 |
| 👀 张望 | 眼睛快速左右扫视 |

每种表情都有独立的 draw 函数（静态帧）和 anim 函数（完整动画序列）。

### 触摸交互（TTP223 传感器）

新增 TTP223 电容触摸传感器，接 **GPIO 5**。

- **摸一下** → 随机播放正面表情（开心、亲亲、Squish、眨眼）
- **25 秒内摸 5 次以上** → 触摸过载 → 晕眼表情
- **最后一次触摸后 20 秒** → 自动恢复时间逻辑
- **动画播放中触摸** → 立刻打断当前动画并响应（通过 `animDelay()` 可中断延迟机制实现）

### 时间感知（NTP 同步）

连接家用 WiFi 后通过 NTP 同步时间，实现基于时间段的表情调度：

| 时间段 | 行为 |
|---|---|
| 23:00 – 08:00 | 睡觉模式（持续播放） |
| 12:00 – 13:00 | 犯困模式（持续播放） |
| 其余时段 | 加权随机轮播 |

### 自动轮巡系统

`loop()` 改为完整状态机：

1. 处理网页请求
2. 检测触摸输入
3. 检查手动模式超时（20 秒）
4. 应用时间强规则
5. 当前表情连播（10 次后切换）
6. 加权随机选择下一个表情

表情概率分布：普通 19%、开心 9%、无聊 8%、Squish 8%、思考 8%、不爽 8%、生气 7%、伤心 7%、眨眼 5%、亲亲 5%、张望 3%、睡觉 3%、犯困 2%、困惑 2%、晕 2%、死机 2%。

### WiFi 模式变更

从纯 AP 热点模式改为 **STA 模式**（连接家用 WiFi），连接失败时回退到原版热点模式（`ClaWD-Mochi` / `clawd1234`）。连接成功后屏幕显示实际分配的 IP 地址。

---

## 额外硬件

在[原版零件清单](https://github.com/yousifamanuel/clawd-mochi#parts-list)基础上只多一个零件：

| 零件 | 规格 | 约价格 |
|---|---|---|
| TTP223 触摸传感器 | 电容式触摸模块 | ~¥2 |

接线：**SIG → GPIO 5**，**VCC → 3V3**，**GND → GND**。

其余硬件（ESP32-C3、ST7789 屏幕、接线、3D 外壳）与原版完全一致。

---

## 安装

按照[原版安装指南](https://github.com/yousifamanuel/clawd-mochi#software-setup)配置 Arduino IDE、开发板支持和库。

上传前编辑 `clawd_mochi_touchable.ino`，替换 WiFi 信息：

```cpp
const char* WIFI_SSID = "你的WiFi名称";
const char* WIFI_PASS = "你的WiFi密码";
```

> **重要：** 烧录前务必设置 **Tools → USB CDC On Boot → Enabled**，否则开发板无法被正确识别。

---

## 文件结构

| 文件 | 说明 |
|---|---|
| `clawd_mochi_touchable.ino` | 修改版固件（触摸 + 表情 + 时间逻辑） |
| `clawd_mochi.ino` | [@yousifamanuel](https://github.com/yousifamanuel) 的原版固件（未修改） |

---

## 致谢

本项目 fork 自 [yousifamanuel/clawd-mochi](https://github.com/yousifamanuel/clawd-mochi)。所有硬件设计、3D 模型、接线方案、网页控制器和原版固件均由 [@yousifamanuel](https://github.com/yousifamanuel) 创作。本 fork 仅修改了 Arduino 固件部分。

## 许可证

与原版一致 — 代码使用 [MIT License](LICENSE)，3D 模型和媒体素材使用 **CC BY-NC-SA 4.0**。
