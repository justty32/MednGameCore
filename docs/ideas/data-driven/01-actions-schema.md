# actions.json schema 設計（未實作的前瞻提案）

> 日期：2026-06-11（內容於 2026-06-21 回合系統重構後校正）
> 定位：**若要重新引入資料驅動動作/技能**，schema 該長怎樣——版本化、驗證與
> 錯誤回報、原語擴充路線，以及「原語組合 vs 表達式/腳本」的取捨分析。

> **狀態（2026-06-21）：本文描述的系統目前不存在。** 2026-06-21 重構移除了
> ActionDef/ActionLibrary、`data/actions.json`（fireball/heal/meteor/venom）、
> DoT、nova、EffectRegistry。現在 Action 只是極簡 enum
> （`Idle`/`Move`/`Wait`，攻擊＝撞擊），由 `apply_action` 直接結算。
> 本文保留作為「**未來在最簡 TurnEngine／apply_action 之上重新引入資料驅動動作**」
> 的 schema 設計參考，而非現況描述。

現況依賴：`src/core/turn/action.h`（`enum class ActionKind { Idle, Move, Wait }`）、
`src/core/turn/apply_action.h`/`.cpp`（撞擊攻擊、拾取、下樓的硬編結算）。
若實作本提案，新增的會是：`data/actions.json` ＋ 某種 `ActionLibrary`/效果原語層。

---

## 1. 重新引入的前提與目標

要把動作資料化，得先在 apply_action 之上補一個**可組合效果原語層**：目前
`apply_action` 只認 enum，沒有「效果」的概念。重新引入時的設計目標：

- 從零定義 schema，**不恢復舊系統**（舊 ActionDef 綁在已移除的能量/詠唱模型，
  欄位如 `on_channel`/`on_resolve`/`dot_kind` 都假設那套時間模型存在，不再適用）。
- 載入失敗**不可靜默**：要回報「哪個檔、哪筆、哪欄」（避免舊系統 `catch(...)`
  退回硬編、使用者以為改了 JSON 沒生效的痛點）。
- 先有 `schema_version`，再增欄位。
- 載入後做語意驗證（範圍、重名、引用存在）。

下文的 schema 草案即為達成上述目標的設計。

## 2. schema_version 欄位（優先序：高，重新引入時的第一步）

頂層加版本，與 actions 並列：

```json
{ "schema_version": 2, "actions": [ ... ] }
```

- 載入流程：先讀 `schema_version`（缺欄位視為 1）→ 按版本分派解析。
- 規約：**加欄位（有預設值）不升版；改名/改語意/刪欄位才升版**。
- 配套：ActionDef 的序列化若用 cereal，要解決「optional NVP」問題——
  cereal JSON 原生沒有 optional NVP，務實做法是自寫小工具
  `load_or(ar, "key", value, default)`（try-catch 包單欄位）或乾脆
  換手寫 JSON 解析（見 §6）。**目標：JSON 只需寫非預設欄位**，
  每筆 action 只寫 3-5 個有意義的欄位，可讀性高。

## 3. 驗證與錯誤訊息品質（優先序：高）

分兩層：

1. **解析層**：失敗時回報「哪個檔、哪筆 action（index 或已讀到的 name）、哪個欄位」。
   `load_json` 簽名回傳 `LoadResult { bool ok; std::vector<std::string> errors; }`，
   呼叫端（`zone_world_gd.cpp`）把 errors 餵進 trace/debug log 管線
   （`get_debug_log` → Godot console），開發者直接看到。
2. **語意層**（解析成功後逐筆檢查）：
   - `weight >= 1`、`radius >= 1`、`dot_kind ∈ {0,1,2}`、各數值非負
   - name 非空、不重複（reindex 時順手檢查）
   - 至少一個原語非零（全零的 action 是資料錯誤）
   - 違規 → 該筆剔除 + error 訊息，**其餘筆照常載入**（部分成功優於全檔退回）

後備規則改為：解析層全敗才 `load_defaults()`，且必吐 warning（不變原則 3）。

## 4. 原語擴充：傷害、投射物、召喚、位移（優先序：中）

採「欄位＝原語開關＋參數」模式。**起點是先把目前 apply_action 寫死的撞擊攻擊
抽成一個 `damage` 原語**，再逐個加擴充原語（以下 C++ 端皆為新寫）：

| 原語 | 欄位草案 | C++ 端工作 |
|---|---|---|
| 直接傷害（撞擊/近戰） | `damage`（傷害量） | 把現有 apply_action 撞擊扣血抽成 `resolve_damage`，作為其他原語的基礎 |
| 位移（dash/knockback） | `displace`（格數，正=自己衝刺，負=擊退命中者） | 新 `resolve_displace`：沿方向逐格檢查 walkable，撞牆截斷；複用 `move_dir.h` 編碼 |
| 投射物 | `projectile_range`、`projectile_damage` | 新 `resolve_projectile`：沿方向 raycast 首個 actor/牆；回合制下「即時命中的直線攻擊」即可，不需要飛行實體 |
| 召喚 | `summon_proto`（字串，引用 prototype id）、`summon_count` | **依賴 02 的 PrototypeDef**——召喚是「prototype 資料化」的第一個跨檔引用，建議在 02 落地後做 |

> 註：DoT/狀態效果（燃燒/中毒/回復）與 nova 範圍攻擊是更後面的主題，且需要先
> 引入「狀態元件 + 每回合 tick」機制（這些在 2026-06-21 重構中連同舊系統一起移除）。
> 不要假設它們現成可用。

風險控制：欄位變多時可讀性會崩，屆時把 ActionDef 重構成
`effects: [ { "kind": "damage", ... }, { "kind": "displace", ... } ]` 陣列形式
（一個 action 多個效果步驟）——這是 schema_version 升版的天然時機。

## 5. 表達式/腳本 vs 原語組合：取捨分析（優先序：低，傾向不做）

| 面向 | 原語組合（建議路線） | 表達式（如 `damage = 2*level+3`） | 腳本（Lua/quickjs） |
|---|---|---|---|
| 可測試性 | doctest 直接測原語，極佳 | 要測表達式引擎本身 | 要測 binding 層，最差 |
| 存檔相容 | def 不入存檔，無影響 | 同左 | 腳本狀態難序列化 |
| modder 上限 | 只能組合既有原語 | 數值公式自由 | 行為自由 |
| 失敗模式 | 載入期即驗證 | 執行期才爆 | 執行期爆 + 沙箱問題 |
| 依賴 | 零 | 小型 eval 器（可自寫） | 外部 VM、跨平台建置成本 |

**結論建議**：本專案是「自家內容為主、modding 為遠期」，原語組合的天花板
（搭配 §4 的 effects 陣列，建立在 apply_action 原語層之上）足以涵蓋
roguelike 技能 9 成需求；
**腳本不做**。表達式可考慮一個極窄的折衷：允許數值欄位寫
`"damage": "2*floor+3"` 字串、載入期編譯成係數表（只支援 `a*var+b` 線性式），
給「隨樓層成長的怪物技能」用——這在 02/03 樓層縮放需求出現時再評估。

## 6. cereal 之外的選項（備忘）

cereal JSON 適合「C++ 結構鏡像」，不適合「寬鬆 schema + 好錯誤訊息」。
若 §2/§3 在 cereal 上做得太彆扭，換 nlohmann/json（header-only，FetchContent 一行）
手寫 `from_json(const json&, ActionDef&, LoadResult&)` 是務實退路——
**只換 data 檔解析，存檔仍走 cereal binary**，兩者本來就是不同管線
（`src/core/serialize/` 不受影響）。
