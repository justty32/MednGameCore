# 角色成長：屬性模型與用進廢退

> 日期：2026-06-11
> 定位：為戰鬥公式、技能升級、裝備欄建立角色長期成長模型，但避免太早導入等級膨脹。

---

## 1. 設計傾向

暫不做傳統 character level。gamecore-zone 的核心樂趣比較接近「行動選擇與時間模型」，
若直接用等級壓數值，會掩蓋 scheduler 與技能取捨。

傾向：**屬性小幅成長 + 技能熟練 + 裝備差異**。

## 2. 屬性模型（優先序：中）

```cpp
struct AttributesComponent {
    int might{10};    // 近戰、負重
    int finesse{10};  // 命中、閃避、暴擊
    int focus{10};    // 法術、抗性、詠唱穩定
    int vigor{10};    // HP、DoT 抗性
};
```

CombatStats 可由 attribute + 裝備 + buff 派生，或短期維持 CombatStats 為真相，Attributes 只在新系統使用。

## 3. 用進廢退（優先序：中低）

每次行動累積 usage XP：

- 近戰命中 → might / melee skill。
- 閃避成功 → finesse。
- 施法完成 → focus / 該技能熟練。
- 承受 DoT 並存活 → vigor 小幅成長。

成長要非常慢，且有 diminishing returns，避免玩家刻意刷。

## 4. 技能點折衷

比完全用進廢退更可控的做法：

- 戰鬥結束給少量 skill point。
- 玩家把點數投到 known skill level。
- usage XP 只提供小折扣或解鎖資格。

這比較容易平衡，也能支撐 `ActionDef.levels`。

## 5. 不做的事

- 不做大幅等級壓制。
- 不讓玩家靠原地空放技能刷成長；需要命中、有效治療或戰鬥結算。
- 不在屬性模型穩定前做複雜裝備詞綴。
