# 目標選擇：方向、游標與範圍預覽

> 日期：2026-06-11
> 定位：為 bolt/cone/smite_target 等非 self-nova 技能設計前端選目標流程。

---

## 1. 為什麼需要 targeting

現在技能全部是以自己為中心的 nova，按鍵即可施放。當 `ActionDef.shape` 擴成 bolt、cone、指定格後，玩家需要先表達「往哪裡」或「打哪格」。

## 2. 第一階段：方向 targeting（優先序：中高）

流程：

```text
按 skill -> 進 direction_targeting -> 方向鍵 -> submit_skill(def, dir)
```

適用：

- bolt
- cone
- dash / knockback 類方向技能

前端預覽：

- 用高亮格顯示直線或扇形。
- 牆後格子不高亮。
- 若第一個命中者可算出，顯示目標框。

## 3. 第二階段：游標 targeting（優先序：中）

流程：

```text
按 skill -> 游標出現在 hero 或最近敵人 -> 移動游標 -> confirm -> submit_skill(def, packed_xy)
```

適用：

- smite_target
- 丟物品
- 檢視敵人
- 未來遠程互動

游標狀態純前端保存；core 只收到打包後座標。

## 4. 預覽資料來源

短期可前端重算簡單幾何；中期建議 core export：

```gdscript
zone_world.preview_action(actor_id, action_def, param) -> Array[Vector2i]
```

原因：牆阻擋、FOV、友軍傷害、射線停在第一個 actor 等規則最終都在 core，預覽若與 resolve 不一致會傷手感。

## 5. 不做的事

- 不做自由像素瞄準；本遊戲是格子戰術。
- 不讓預覽承諾 RNG 結果；只預覽範圍與可能目標，不預覽命中/暴擊。
- 不在 targeting 第一版處理滑鼠 hover 全功能。
